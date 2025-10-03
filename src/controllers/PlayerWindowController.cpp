#include "controllers/PlayerWindowController.h"
#include "ui/MainWindow.h"
#include "ui/PlayerWindow.h"
#include "graphics/MapDisplay.h"
#include "utils/SettingsManager.h"
#include "utils/DebugConsole.h"
#include <QAction>
#include <QApplication>
#include <QScreen>
#include <QTimer>

PlayerWindowController::PlayerWindowController(QObject *parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_mapDisplay(nullptr)
    , m_playerWindow(nullptr)
    , m_toggleAction(nullptr)
{
}

PlayerWindowController::~PlayerWindowController()
{
    destroyPlayerWindow();
}

void PlayerWindowController::attachToMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
}

void PlayerWindowController::setMapDisplay(MapDisplay* display)
{
    m_mapDisplay = display;
}

void PlayerWindowController::setToggleAction(QAction* action)
{
    m_toggleAction = action;
}

bool PlayerWindowController::isPlayerWindowVisible() const
{
    return m_playerWindow && m_playerWindow->isVisible();
}

void PlayerWindowController::togglePlayerWindow()
{
    if (m_playerWindow) {
        if (m_playerWindow->isVisible()) {
            m_playerWindow->hide();
            emit playerWindowToggled(false);
            emit requestStatus("Player window closed");
            DebugConsole::info("Player window closed", "PlayerWindow");
        } else {
            m_playerWindow->show();
            m_playerWindow->raise();
            m_playerWindow->activateWindow();
            emit playerWindowToggled(true);
            emit requestStatus("Player window opened");
            DebugConsole::info("Player window opened", "PlayerWindow");
        }
    } else {
        createPlayerWindow();
        if (m_playerWindow) {
            m_playerWindow->show();
            emit playerWindowToggled(true);
            emit requestStatus("Player window created and opened");
            DebugConsole::info("Player window created and shown", "PlayerWindow");
        }
    }

    if (m_toggleAction && m_playerWindow) {
        m_toggleAction->setChecked(m_playerWindow->isVisible());
    }
}

void PlayerWindowController::autoOpenPlayerWindow()
{
    if (!m_mainWindow) return;

    QList<QScreen*> screens = QApplication::screens();
    if (screens.size() > 1) {
        DebugConsole::info(
            QString("Multiple displays detected (%1). Auto-opening player window.")
                .arg(screens.size()),
            "PlayerWindow"
        );

        if (!m_playerWindow) {
            createPlayerWindow();
        }

        if (m_playerWindow) {
            positionPlayerWindow();
            m_playerWindow->show();
            emit playerWindowToggled(true);

            if (m_toggleAction) {
                m_toggleAction->setChecked(true);
            }

            DebugConsole::info("Player window auto-opened on secondary display", "PlayerWindow");
        }
    } else {
        DebugConsole::info("Single display detected. Player window not auto-opened.", "PlayerWindow");
    }
}

void PlayerWindowController::updatePlayerWindow()
{
    if (m_playerWindow && m_playerWindow->isVisible()) {
        m_playerWindow->updateDisplay();
    }
}

void PlayerWindowController::syncWithMainWindow()
{
    if (!m_playerWindow || !m_playerWindow->isVisible()) return;
    if (!m_mapDisplay) return;

    qreal zoom = m_mapDisplay->getZoomLevel();
    m_playerWindow->syncZoom(zoom);

    updatePlayerWindow();
}

void PlayerWindowController::createPlayerWindow()
{
    if (m_playerWindow || !m_mainWindow || !m_mapDisplay) return;

    m_playerWindow = new PlayerWindow(m_mapDisplay, m_mainWindow);

    if (m_mapDisplay) {
        connect(m_mapDisplay, &MapDisplay::fogChanged,
                this, &PlayerWindowController::updatePlayerWindow,
                Qt::UniqueConnection);
    }

    // Connect privacy mode signal to MainWindow
    if (m_mainWindow) {
        connect(m_playerWindow, &PlayerWindow::privacyModeChanged,
                m_mainWindow, &MainWindow::updatePrivacyStatusIndicator);
    }

    QRect savedGeometry = SettingsManager::instance().loadWindowGeometry(
        "PlayerWindow",
        QRect(100, 100, 800, 600)
    );
    m_playerWindow->setGeometry(savedGeometry);

    // Note: PlayerWindow doesn't have a windowClosing signal
    // The window closing will be handled by the closeEvent override in PlayerWindow

    DebugConsole::info("Player window created", "PlayerWindow");
}

void PlayerWindowController::positionPlayerWindow()
{
    if (!m_playerWindow || !m_mainWindow) return;

    QList<QScreen*> screens = QApplication::screens();
    if (screens.size() > 1) {
        QScreen* primaryScreen = screens[0];
        QScreen* secondaryScreen = screens[1];

        if (m_mainWindow->screen() == secondaryScreen) {
            secondaryScreen = primaryScreen;
        }

        QRect screenGeometry = secondaryScreen->geometry();
        int width = screenGeometry.width() * 0.8;
        int height = screenGeometry.height() * 0.8;
        int x = screenGeometry.x() + (screenGeometry.width() - width) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - height) / 2;

        m_playerWindow->setGeometry(x, y, width, height);

        DebugConsole::info(
            QString("Player window positioned on screen at (%1, %2) with size %3x%4")
                .arg(x).arg(y).arg(width).arg(height),
            "PlayerWindow"
        );
    }
}

void PlayerWindowController::destroyPlayerWindow()
{
    if (m_playerWindow) {
        SettingsManager::instance().saveWindowGeometry("PlayerWindow", m_playerWindow->geometry());
        delete m_playerWindow;
        m_playerWindow = nullptr;
    }
}
