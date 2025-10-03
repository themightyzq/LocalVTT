#include "TabsController.h"
#include "graphics/MapDisplay.h"
#include "graphics/SceneManager.h"
#include "utils/MapSession.h"
#include "utils/ErrorHandler.h"
#include "utils/DebugConsole.h"

#include <QTabWidget>
#include <QFileInfo>
#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QPointer>
#include <QMutexLocker>

TabsController::TabsController(QObject* parent)
    : QObject(parent) {}

void TabsController::attach(QTabWidget* tabs, MapDisplay* display, int maxTabs)
{
    m_tabs = tabs;
    m_display = display;
    m_maxTabs = maxTabs;
    if (m_tabs) {
        connect(m_tabs, &QTabWidget::currentChanged, this, &TabsController::onTabChanged);
        connect(m_tabs, &QTabWidget::tabCloseRequested, this, &TabsController::onTabCloseRequested);
    }
}

void TabsController::loadMapFile(const QString& path)
{
    if (!m_tabs || !m_display) {
        ErrorHandler::instance().reportError(
            "Cannot load map: UI components not initialized",
            ErrorLevel::Error
        );
        return;
    }

    // Check if file exists and is readable
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        ErrorHandler::instance().reportError(
            QString("Map file not found: %1").arg(path),
            ErrorLevel::Error
        );
        emit requestStatus(QStringLiteral("Map file not found"), 3000);
        return;
    }

    if (!fileInfo.isReadable()) {
        ErrorHandler::instance().reportError(
            QString("Cannot read map file: %1").arg(path),
            ErrorLevel::Error
        );
        emit requestStatus(QStringLiteral("Cannot read map file"), 3000);
        return;
    }

    // If already open, switch
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i]->filePath() == path) {
            m_tabs->setCurrentIndex(i);
            return;
        }
    }

    if (m_sessions.size() >= m_maxTabs) {
        ErrorHandler::instance().reportError(
            QString("Tab limit reached (%1 tabs open)").arg(m_maxTabs),
            ErrorLevel::Warning
        );
        emit requestStatus(QStringLiteral("Tab limit reached. Close a tab to open a new map."), 3000);
        return;
    }

    createNewTab(path);
}

void TabsController::createNewTab(const QString& filePath)
{
    QFileInfo fi(filePath);
    const qint64 fileSize = fi.size();
    if (fileSize > 1024 * 1024) {
        emit requestShowProgress(fi.fileName(), fileSize);
    }

    // Defer heavy load slightly to keep UI responsive (mirrors existing behavior)
    // CRITICAL FIX: Capture weak pointer to prevent crash if window closes during load
    QPointer<TabsController> weakThis(this);
    QTimer::singleShot(10, this, [weakThis, filePath, fi, fileSize]() {
        // Check if controller still exists (window not closed)
        if (!weakThis) {
            return;  // Window was closed, abort
        }

        // Additional safety check for member variables
        if (!weakThis->m_tabs || !weakThis->m_display) {
            return;  // UI components destroyed, abort
        }

        MapSession* session = nullptr;
        try {
            session = new MapSession(filePath);
            const bool ok = session->loadImage();

            // Check again after potentially long load operation
            if (!weakThis || !weakThis->m_tabs || !weakThis->m_display) {
                delete session;
                return;  // Window closed during load
            }

            emit weakThis->requestHideProgress();

            if (!ok) {
                delete session;
                QString errorMsg = QStringLiteral("Failed to load map: %1").arg(fi.fileName());
                ErrorHandler::instance().reportError(errorMsg, ErrorLevel::Error);
                emit weakThis->requestStatus(errorMsg, 5000);
                return;
            }
        } catch (const std::exception& e) {
            if (session) delete session;
            if (weakThis) {
                emit weakThis->requestHideProgress();
                QString errorMsg = QString("Error loading map: %1").arg(e.what());
                ErrorHandler::instance().reportError(errorMsg, ErrorLevel::Error);
                emit weakThis->requestStatus(errorMsg, 5000);
            }
            return;
        } catch (...) {
            if (session) delete session;
            if (weakThis) {
                emit weakThis->requestHideProgress();
                QString errorMsg = "Unknown error occurred while loading map";
                ErrorHandler::instance().reportError(errorMsg, ErrorLevel::Critical);
                emit weakThis->requestStatus(errorMsg, 5000);
            }
            return;
        }

        weakThis->m_sessions.append(session);
        
        // CRITICAL FIX: Activate the session BEFORE adding the tab
        // This ensures the map is actually loaded when the tab is displayed
        // For the first tab, we need to activate it manually since there's no previous tab
        if (weakThis->m_sessions.size() == 1) {
            DebugConsole::info("First tab - activating session immediately", "Tabs");
            weakThis->m_currentIndex = 0;
            session->activateSession(weakThis->m_display);
        } else {
            DebugConsole::info(QString("Not first tab (have %1 tabs)").arg(weakThis->m_sessions.size()), "Tabs");
        }
        
        // Use the shared display as the tab widget content
        const int tabIndex = weakThis->m_tabs->addTab(weakThis->m_display, weakThis->shortTitle(filePath));
        weakThis->m_tabs->show();
        weakThis->m_display->show();
        weakThis->m_tabs->setCurrentIndex(tabIndex);

        emit weakThis->requestAddRecent(filePath);
        emit weakThis->requestStatus(QStringLiteral("Loaded: %1").arg(fi.fileName()), 5000);
    });
}

void TabsController::onTabChanged(int index)
{
    switchToTab(index);
}

void TabsController::onTabCloseRequested(int index)
{
    closeTab(index);
}

void TabsController::switchToTab(int index)
{
    if (!m_display || !m_tabs) {
        ErrorHandler::instance().reportError(
            "Cannot switch tabs: UI components not initialized",
            ErrorLevel::Warning
        );
        return;
    }

    if (index < 0 || index >= m_sessions.size()) {
        ErrorHandler::instance().reportError(
            QString("Invalid tab index: %1 (have %2 tabs)").arg(index).arg(m_sessions.size()),
            ErrorLevel::Warning
        );
        return;
    }
    
    // If switching to the same tab that's already active, do nothing
    if (index == m_currentIndex) {
        return;
    }

    try {
        // Save and deactivate previous session
        if (m_currentIndex >= 0 && m_currentIndex < m_sessions.size()) {
            MapSession* cur = m_sessions[m_currentIndex];
            if (cur) {
                cur->setZoomLevel(m_display->getZoomLevel());
                cur->setViewCenter(m_display->mapToScene(m_display->rect().center()));
                cur->deactivateSession(m_display);
            }
        }

        m_currentIndex = index;
        MapSession* next = m_sessions[index];
        if (!next) {
            ErrorHandler::instance().reportError(
                "Tab session is null",
                ErrorLevel::Error
            );
            return;
        }

        next->activateSession(m_display);

        emit currentMapPathChanged(next->filePath());
        emit uiChanged();
        emit sceneChanged();
    } catch (const std::exception& e) {
        ErrorHandler::instance().reportError(
            QString("Error switching tabs: %1").arg(e.what()),
            ErrorLevel::Error
        );
    } catch (...) {
        ErrorHandler::instance().reportError(
            "Unknown error while switching tabs",
            ErrorLevel::Critical
        );
    }
}

void TabsController::closeTab(int index)
{
    if (!m_tabs || !m_display) return;
    if (index < 0 || index >= m_sessions.size()) return;

    MapSession* session = m_sessions[index];
    session->deactivateSession(m_display);
    m_sessions.removeAt(index);
    m_tabs->removeTab(index);
    delete session;

    if (m_sessions.isEmpty()) {
        m_tabs->hide();
        m_currentIndex = -1;
        emit currentMapPathChanged(QString());
        QMutexLocker locker(&SceneManager::getSceneMutex());
        if (m_display->scene()) {
            m_display->scene()->clear();
        }
    } else {
        if (index <= m_currentIndex) {
            m_currentIndex--;
        }
        const int newIndex = qMin(index, m_sessions.size() - 1);
        m_tabs->setCurrentIndex(newIndex);
    }
}

QString TabsController::shortTitle(const QString& filePath) const
{
    QFileInfo info(filePath);
    QString base = info.baseName();
    if (base.size() > 20) return base.left(17) + QStringLiteral("...");
    return base;
}

MapSession* TabsController::getCurrentSession() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_sessions.size()) {
        return m_sessions[m_currentIndex];
    }
    return nullptr;
}
