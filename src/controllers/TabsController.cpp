#include "TabsController.h"
#include "graphics/MapDisplay.h"
#include "utils/MapSession.h"
#include "utils/ErrorHandler.h"
#include "utils/DebugConsole.h"
#include "ui/widgets/ThumbnailCache.h"

#include <QTabBar>
#include <QFileInfo>
#include <QBuffer>
#include <QByteArray>
#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QPointer>

TabsController::TabsController(QObject* parent)
    : QObject(parent) {}

void TabsController::attach(QTabBar* tabBar, MapDisplay* display, int maxTabs)
{
    m_tabBar = tabBar;
    m_display = display;
    m_maxTabs = maxTabs;
    if (m_tabBar) {
        connect(m_tabBar, &QTabBar::currentChanged, this, &TabsController::onTabChanged);
        connect(m_tabBar, &QTabBar::tabCloseRequested, this, &TabsController::onTabCloseRequested);
    }
}

void TabsController::loadMapFile(const QString& path)
{
    if (!m_tabBar || !m_display) {
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
            m_tabBar->setCurrentIndex(i);
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
    QTimer::singleShot(10, this, [weakThis, filePath, fi]() {
        // Check if controller still exists (window not closed)
        if (!weakThis) {
            return;  // Window was closed, abort
        }

        // Additional safety check for member variables
        if (!weakThis->m_tabBar || !weakThis->m_display) {
            return;  // UI components destroyed, abort
        }

        MapSession* session = nullptr;
        try {
            session = new MapSession(filePath);
            const bool ok = session->loadImage();

            // Check again after potentially long load operation
            if (!weakThis || !weakThis->m_tabBar || !weakThis->m_display) {
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
        }

        // Deactivate current session before adding new one
        if (weakThis->m_currentIndex >= 0 && weakThis->m_currentIndex < weakThis->m_sessions.size()) {
            MapSession* currentSession = weakThis->m_sessions[weakThis->m_currentIndex];
            if (currentSession) {
                currentSession->setZoomLevel(weakThis->m_display->getZoomLevel());
                currentSession->setViewCenter(weakThis->m_display->mapToScene(weakThis->m_display->rect().center()));
                currentSession->deactivateSession(weakThis->m_display);
            }
        }

        weakThis->m_sessions.append(session);
        const int newIndex = weakThis->m_sessions.size() - 1;

        // CRITICAL FIX: Always activate the session explicitly
        // We can't rely on setCurrentIndex triggering currentChanged because:
        // 1. When we add the same widget (m_display) to a new tab, Qt may auto-select it
        // 2. If Qt already made the new tab current, setCurrentIndex won't emit currentChanged
        DebugConsole::info(QString("Activating session for tab %1").arg(newIndex), "Tabs");
        weakThis->m_currentIndex = newIndex;
        session->activateSession(weakThis->m_display);

        // Add tab to the tab bar (just adds a label, MapDisplay is managed separately)
        const int tabIndex = weakThis->m_tabBar->addTab(weakThis->shortTitle(filePath));
        weakThis->m_tabBar->show();
        weakThis->m_display->show();

        // Set tab tooltip with thumbnail preview
        weakThis->setTabTooltipWithThumbnail(tabIndex, filePath);

        // Set current index (may not trigger signal if already current, but that's fine now)
        weakThis->m_tabBar->setCurrentIndex(tabIndex);

        // Emit signals to update UI (currentMapPathChanged, uiChanged, sceneChanged)
        emit weakThis->currentMapPathChanged(filePath);
        emit weakThis->uiChanged();
        emit weakThis->sceneChanged();

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
    if (!m_display || !m_tabBar) {
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
    }
}

void TabsController::closeTab(int index)
{
    if (!m_tabBar || !m_display) return;
    if (index < 0 || index >= m_sessions.size()) return;

    MapSession* session = m_sessions[index];
    session->deactivateSession(m_display);
    m_sessions.removeAt(index);
    m_tabBar->removeTab(index);
    delete session;

    if (m_sessions.isEmpty()) {
        m_tabBar->hide();
        m_currentIndex = -1;
        emit currentMapPathChanged(QString());
        if (m_display->scene()) {
            m_display->scene()->clear();
        }
    } else {
        if (index <= m_currentIndex) {
            m_currentIndex--;
        }
        const int newIndex = qMin(index, m_sessions.size() - 1);
        m_tabBar->setCurrentIndex(newIndex);
    }
}

QString TabsController::shortTitle(const QString& filePath) const
{
    QFileInfo info(filePath);
    QString base = info.baseName();
    if (base.size() > 20) return base.left(17) + QStringLiteral("...");
    return base;
}

void TabsController::setTabTooltipWithThumbnail(int tabIndex, const QString& filePath)
{
    if (!m_tabBar || tabIndex < 0) return;

    QFileInfo info(filePath);

    // Try to get thumbnail from cache
    QPixmap thumbnail = ThumbnailCache::instance().getThumbnail(filePath);

    QString tooltipHtml;
    if (!thumbnail.isNull()) {
        // Scale thumbnail to a reasonable tooltip size (150px max)
        QPixmap scaled = thumbnail.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Convert pixmap to base64 for embedding in HTML tooltip
        QByteArray imageData;
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        scaled.save(&buffer, "PNG");

        QString base64Image = QString::fromLatin1(imageData.toBase64());

        tooltipHtml = QString(
            "<div style='text-align: center;'>"
            "<img src='data:image/png;base64,%1' style='max-width: 150px;'/><br/>"
            "<b>%2</b><br/>"
            "<span style='color: #888;'>%3</span>"
            "</div>"
        ).arg(base64Image, info.fileName(), info.absolutePath());
    } else {
        // No thumbnail available yet, just show text
        tooltipHtml = QString(
            "<b>%1</b><br/>"
            "<span style='color: #888;'>%2</span>"
        ).arg(info.fileName(), info.absolutePath());
    }

    m_tabBar->setTabToolTip(tabIndex, tooltipHtml);
}

MapSession* TabsController::getCurrentSession() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_sessions.size()) {
        return m_sessions[m_currentIndex];
    }
    return nullptr;
}
