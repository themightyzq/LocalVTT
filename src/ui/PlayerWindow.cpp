#include "ui/PlayerWindow.h"
#include "graphics/MapDisplay.h"
#include "graphics/FogOfWar.h"
#include "graphics/SceneManager.h"
#include "utils/SettingsManager.h"
#include "utils/AnimationHelper.h"
#include "utils/SecureWindowRegistry.h"
#include <iostream>
#include <QMutexLocker>
#include <QMenuBar>
#include <QStatusBar>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QShowEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QApplication>
#include <QWindow>
#include <QContextMenuEvent>
#include <QMenu>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QVBoxLayout>

PlayerWindow::PlayerWindow(MapDisplay* sharedDisplay, QWidget *parent)
    : QMainWindow(parent)
    , m_sharedDisplay(sharedDisplay)
    , m_playerView(nullptr)
    , m_windowAnimation(nullptr)
    , m_currentScreen(nullptr)
    , m_autoFitEnabled(true)
    , m_blackoutOverlay(nullptr)
    , m_intermissionOverlay(nullptr)
    , m_intermissionLabel(nullptr)
    , m_privacyModeActive(false)
    , m_tripleClickTimer(new QTimer(this))
    , m_clickCount(0)
    , m_refreshTimer(new QTimer(this))
    , m_autoFitTimer(new QTimer(this))
{
    setObjectName("PlayerWindow");

    SecureWindowRegistry::instance().registerWindow(this, SecureWindowRegistry::PlayerWindow);

    setWindowFlags(windowFlags() | Qt::Window);

    // Create a view that shares the same scene as the main window
    m_playerView = new MapDisplay(this);
    if (m_sharedDisplay) {
        m_playerView->shareScene(m_sharedDisplay);

        // CRITICAL FIX: Share MainWindow reference for fog tool synchronization
        // This enables PlayerWindow to access current fog tool mode for proper cursor updates
        if (m_sharedDisplay->getMainWindow()) {
            m_playerView->setMainWindow(m_sharedDisplay->getMainWindow());
        }
    }
    
    // Disable independent zoom controls for player window
    m_playerView->setZoomControlsEnabled(false);
    
    setCentralWidget(m_playerView);

    // Apply premium theme
    setEnhancedStyling();

    // Apply themed menu bar

    // Minimal menu for player window
    QMenu* viewMenu = menuBar()->addMenu("&View");

    // Auto-Fit to Screen option
    QAction* autoFitAction = new QAction("&Auto-Fit to Screen", this);
    autoFitAction->setCheckable(true);
    autoFitAction->setChecked(m_autoFitEnabled);
    autoFitAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    autoFitAction->setStatusTip("Automatically fit map to screen when window is moved to different monitor");
    connect(autoFitAction, &QAction::toggled, [this](bool checked) {
        m_autoFitEnabled = checked;
        if (checked) {
            autoFitToScreen();
        }
    });
    viewMenu->addAction(autoFitAction);

    QAction* fullScreenAction = new QAction("&Full Screen", this);
    fullScreenAction->setShortcut(QKeySequence("F11"));
    connect(fullScreenAction, &QAction::triggered, [this]() {
        // CRITICAL FIX: Simple fullscreen toggle without complex animations that cause crashes
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });
    viewMenu->addAction(fullScreenAction);

    viewMenu->addSeparator();

    // Add window opacity control
    QMenu* opacityMenu = viewMenu->addMenu("Window Opacity");
    QStringList opacityLevels = {"100%", "90%", "80%", "70%"};
    qreal opacityValues[] = {1.0, 0.9, 0.8, 0.7};

    for (int i = 0; i < opacityLevels.size(); ++i) {
        QAction* opacityAction = new QAction(opacityLevels[i], this);
        qreal opacity = opacityValues[i];
        connect(opacityAction, &QAction::triggered, [this, opacity]() {
            // Use AnimationHelper for consistent timing (200ms per CLAUDE.md)
            QPropertyAnimation* anim = new QPropertyAnimation(this, "windowOpacity");
            anim->setDuration(AnimationHelper::FADE_DURATION);
            anim->setStartValue(windowOpacity());
            anim->setEndValue(opacity);
            anim->setEasingCurve(AnimationHelper::smoothEasing());
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        });
        opacityMenu->addAction(opacityAction);
    }

    // Minimalist status bar with theme
    statusBar()->showMessage("Player Display");
    
    // Create privacy overlays
    createPrivacyOverlays();

    // Setup triple-click detection timer
    m_tripleClickTimer->setSingleShot(true);
    connect(m_tripleClickTimer, &QTimer::timeout, [this]() {
        m_clickCount = 0; // Reset count after timeout
    });

    // PRIORITY 5 FIX: Setup consolidated refresh timers to prevent timer storms
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, &PlayerWindow::forceRefresh);

    m_autoFitTimer->setSingleShot(true);
    connect(m_autoFitTimer, &QTimer::timeout, this, &PlayerWindow::autoFitToScreen);

    // Load window geometry from settings
    SettingsManager& settings = SettingsManager::instance();
    QRect defaultGeometry(150, 150, 1024, 768);
    QRect savedGeometry = settings.loadWindowGeometry("PlayerWindow", defaultGeometry);
    setGeometry(savedGeometry);
}

PlayerWindow::~PlayerWindow()
{
    // Save window geometry before shutdown
    SettingsManager::instance().saveWindowGeometry("PlayerWindow", geometry());
    
    // Clean shutdown
    if (m_windowAnimation) {
        m_windowAnimation->stop();
        m_windowAnimation->deleteLater();  // Use deleteLater for safer cleanup
        m_windowAnimation = nullptr;
    }
}

void PlayerWindow::setEnhancedStyling()
{
    // Premium minimalist styling - let ThemeManager handle most of it
    // Only add window-specific enhancements here
    QString additionalStyle = QString(
        "QMainWindow {"
        "   background: #1a1a1a;"
        "}"
        "QGraphicsView {"
        "   background: #1a1a1a;"
        "   border: none;"
        "}"
    );
    setStyleSheet(styleSheet() + additionalStyle);
}

void PlayerWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Don't animate on show - can cause crashes if scene is not ready
    setWindowOpacity(1.0);

    // Store initial screen and ALWAYS auto-fit for TV display
    m_currentScreen = screen();
    // CRITICAL FIX: Always fit to screen on show - this is a TV display
    scheduleAutoFit();

    // CRITICAL FIX: Force full refresh when window is shown to prevent black screen and preserve fog
    // Use immediate refresh to ensure fog is visible right away
    forceRefresh();

    // CRITICAL FIX: Also schedule another refresh shortly after to handle any timing issues
    // This ensures fog is properly rendered even if the window manager hasn't finished setup
    scheduleRefresh();
}

void PlayerWindow::closeEvent(QCloseEvent *event)
{
    // Clean shutdown without animation
    if (m_windowAnimation) {
        m_windowAnimation->stop();
    }
    QMainWindow::closeEvent(event);
}

void PlayerWindow::animateWindowTransition(bool showing)
{
    // Disabled - animations on window show/hide can cause crashes
    // Keep method for compatibility but do nothing
    Q_UNUSED(showing);
}

void PlayerWindow::syncZoom(qreal zoomLevel, const QPointF& centerPoint)
{
    // CRITICAL FIX: Player window NEVER syncs zoom from DM window
    // Player window always maintains fit-to-screen for TV display
    // DM can zoom freely without affecting player view
    Q_UNUSED(zoomLevel);
    Q_UNUSED(centerPoint);
    // No-op: Player window ignores all zoom sync requests
}

void PlayerWindow::fitToView()
{
    // CRITICAL FIX: Player window ALWAYS fits to screen
    // This is the TV display - it should fill the screen at all times
    if (m_playerView) {
        m_playerView->fitMapToView();
    }
}

void PlayerWindow::updateToolCursor()
{
    // Synchronize tool cursor with DM window
    if (m_playerView) {
        m_playerView->updateToolCursor();
    }
}

void PlayerWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    SettingsManager::instance().saveWindowGeometry("PlayerWindow", geometry());

    // Check if we moved to a different monitor
    checkForMonitorChange();

    // CRITICAL FIX: Ensure fog persists after window move
    // Use a small delay to let the window settle
    scheduleRefresh();
}

void PlayerWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    SettingsManager::instance().saveWindowGeometry("PlayerWindow", geometry());

    // CRITICAL FIX: Force complete refresh to preserve fog during resize
    // Use immediate refresh to prevent black screen
    forceRefresh();

    // Also schedule a delayed refresh to handle timing issues
    scheduleRefresh();

    // CRITICAL FIX: Always auto-fit when resized - TV display must fill screen
    scheduleAutoFit();
}

bool PlayerWindow::event(QEvent *event)
{
    // Handle any special events if needed
    return QMainWindow::event(event);
}

void PlayerWindow::checkForMonitorChange()
{
    // Get the current screen for this window
    QScreen* newScreen = screen();

    if (newScreen != m_currentScreen) {
        m_currentScreen = newScreen;

        if (m_currentScreen && m_autoFitEnabled) {
            // Window moved to different monitor - auto-fit the map
            statusBar()->showMessage(QString("Moved to %1 - Auto-fitting map...").arg(m_currentScreen->name()), 3000);

            // Delay slightly to allow window to settle on new screen
            scheduleAutoFit();
        }
    }
}

void PlayerWindow::autoFitToScreen()
{
    QMutexLocker locker(&SceneManager::getSceneMutex());

    if (m_playerView && m_playerView->scene() && !m_playerView->scene()->items().isEmpty()) {
        QList<QGraphicsItem*> items = m_playerView->scene()->items();
        QGraphicsPixmapItem* mapItem = nullptr;
        for (QGraphicsItem* item : items) {
            if (auto* pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(item)) {
                mapItem = pixmapItem;
                break;
            }
        }

        if (mapItem) {
            m_playerView->fitInView(mapItem, Qt::KeepAspectRatio);
        }

        locker.unlock();

        if (m_currentScreen) {
            QRect screenGeometry = m_currentScreen->geometry();
            statusBar()->showMessage(QString("Auto-fitted to %1 (%2x%3)")
                                   .arg(m_currentScreen->name())
                                   .arg(screenGeometry.width())
                                   .arg(screenGeometry.height()), 5000);
        }
    }
}

void PlayerWindow::contextMenuEvent(QContextMenuEvent *event)
{
    // Create context menu for quick player window controls
    QMenu contextMenu(this);

    // Fit to Screen action
    QAction* fitAction = contextMenu.addAction("🖥️ Fit to Screen");
    fitAction->setShortcut(QKeySequence("F"));
    connect(fitAction, &QAction::triggered, [this]() {
        autoFitToScreen();
    });

    // Full Screen toggle
    QAction* fullScreenAction = contextMenu.addAction("⛶ Toggle Full Screen");
    fullScreenAction->setShortcut(QKeySequence("F11"));
    fullScreenAction->setCheckable(true);
    fullScreenAction->setChecked(isFullScreen());
    connect(fullScreenAction, &QAction::triggered, [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });

    contextMenu.addSeparator();

    // Auto-Fit toggle
    QAction* autoFitAction = contextMenu.addAction("Auto-Fit on Monitor Change");
    autoFitAction->setCheckable(true);
    autoFitAction->setChecked(m_autoFitEnabled);
    connect(autoFitAction, &QAction::triggered, [this](bool checked) {
        m_autoFitEnabled = checked;
        if (checked) {
            autoFitToScreen();
        }
        statusBar()->showMessage(QString("Auto-fit %1").arg(checked ? "enabled" : "disabled"), 3000);
    });

    // Execute menu at cursor position
    contextMenu.exec(event->globalPos());
}

void PlayerWindow::keyPressEvent(QKeyEvent *event)
{
    // Handle ESC key for privacy mode exit or fullscreen exit
    if (event->key() == Qt::Key_Escape) {
        if (m_privacyModeActive) {
            deactivatePrivacyMode();
            return;
        }
        if (isFullScreen()) {
            showNormal();
            statusBar()->showMessage("Exited fullscreen mode", 2000);
            return;
        }
    }

    // Handle F11 to toggle fullscreen
    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) {
            showNormal();
            statusBar()->showMessage("Exited fullscreen mode", 2000);
        } else {
            showFullScreen();
            statusBar()->showMessage("Entered fullscreen mode", 2000);
        }
        return;
    }

    // Pass event to base class for default handling
    QMainWindow::keyPressEvent(event);
}

void PlayerWindow::mousePressEvent(QMouseEvent *event)
{
    // Handle triple-click detection for emergency blackout
    if (event->button() == Qt::LeftButton) {
        m_clickCount++;

        if (m_clickCount == 1) {
            // Start triple-click timer
            m_tripleClickTimer->start(TRIPLE_CLICK_TIMEOUT);
        } else if (m_clickCount >= 3) {
            // Triple-click detected - activate emergency blackout
            onTripleClickDetected();
            m_clickCount = 0;
            m_tripleClickTimer->stop();
            return;
        }
    }

    QMainWindow::mousePressEvent(event);
}

void PlayerWindow::onTripleClickDetected()
{
    // Emergency blackout activated by triple-click
    activateBlackout();
    statusBar()->showMessage("🔒 Emergency Blackout Activated - Press Escape to exit", 5000);
}

void PlayerWindow::createPrivacyOverlays()
{
    // Create blackout overlay - solid black
    m_blackoutOverlay = new QWidget(this);
    m_blackoutOverlay->setStyleSheet("background-color: #000000;");
    m_blackoutOverlay->hide();

    // Create intermission overlay - professional styling
    m_intermissionOverlay = new QWidget(this);
    m_intermissionOverlay->setStyleSheet(
        "background-color: #1a1a1a;"
        "border: 2px solid #4A90E2;"
    );

    // Create intermission label with professional styling
    m_intermissionLabel = new QLabel(m_intermissionOverlay);
    m_intermissionLabel->setText("INTERMISSION");
    m_intermissionLabel->setAlignment(Qt::AlignCenter);
    m_intermissionLabel->setStyleSheet(
        "color: #E0E0E0;"
        "font-size: 48px;"
        "font-weight: bold;"
        "background: transparent;"
        "border: none;"
        "letter-spacing: 4px;"
    );

    // Layout for intermission overlay
    QVBoxLayout* layout = new QVBoxLayout(m_intermissionOverlay);
    layout->addWidget(m_intermissionLabel);
    layout->setContentsMargins(40, 40, 40, 40);

    m_intermissionOverlay->hide();
}

void PlayerWindow::changeEvent(QEvent *event)
{
    // CRITICAL FIX: Handle window state changes to prevent black screen and fog loss
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *stateEvent = static_cast<QWindowStateChangeEvent*>(event);

        // Check if window was minimized and now restored
        if ((stateEvent->oldState() & Qt::WindowMinimized) &&
            !(windowState() & Qt::WindowMinimized)) {
            // Window was restored from minimized state - force refresh
            scheduleRefresh();
        }
        // CRITICAL FIX: Handle maximize/restore to preserve fog
        else if ((stateEvent->oldState() & Qt::WindowMaximized) !=
                 (windowState() & Qt::WindowMaximized)) {
            // Window maximized or restored - force immediate refresh to preserve fog
            // Use immediate update to prevent fog disappearing
            forceRefresh();

            // Also trigger auto-fit if enabled when maximizing
            if ((windowState() & Qt::WindowMaximized) && m_autoFitEnabled) {
                scheduleAutoFit();
            }
        }
        // CRITICAL FIX: Handle fullscreen transitions
        else if ((stateEvent->oldState() & Qt::WindowFullScreen) !=
                 (windowState() & Qt::WindowFullScreen)) {
            // Entering or exiting fullscreen - force refresh
            scheduleRefresh();
        }
    }

    QMainWindow::changeEvent(event);
}

void PlayerWindow::updateDisplay()
{
    forceRefresh();
}

void PlayerWindow::forceRefresh()
{
    std::cerr << "\n=== [PlayerWindow::forceRefresh] START - NEW ARCHITECTURE ===" << std::endl;

    if (!m_playerView || !m_sharedDisplay) {
        std::cerr << "[PlayerWindow::forceRefresh] ERROR: Null pointers - playerView="
                  << (m_playerView ? "OK" : "NULL") << ", sharedDisplay="
                  << (m_sharedDisplay ? "OK" : "NULL") << std::endl;
        std::cerr.flush();
        return;
    }

    QImage sourceImage = m_sharedDisplay->getCurrentMapImage();
    if (sourceImage.isNull()) {
        std::cerr << "[PlayerWindow::forceRefresh] WARNING: No map loaded in DM window yet" << std::endl;
        std::cerr.flush();
        return;
    }

    std::cerr << "[PlayerWindow::forceRefresh] Source image size: " << sourceImage.width() << "x" << sourceImage.height() << std::endl;
    std::cerr << "[PlayerWindow::forceRefresh] Copying map to PlayerWindow..." << std::endl;
    std::cerr.flush();

    m_playerView->copyMapFrom(m_sharedDisplay);

    if (m_playerView->scene()) {
        std::cerr << "[PlayerWindow::forceRefresh] PlayerView scene items: "
                  << m_playerView->scene()->items().count() << std::endl;
    }

    std::cerr << "[PlayerWindow::forceRefresh] Map copy complete, updating viewport..." << std::endl;
    if (m_playerView->viewport()) {
        m_playerView->viewport()->update();
    }
    m_playerView->update();

    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);

    std::cerr << "=== [PlayerWindow::forceRefresh] END - Display updated ===" << std::endl;
    std::cerr.flush();

    statusBar()->showMessage("Display refreshed", 2000);
}

void PlayerWindow::activateBlackout()
{
    m_privacyModeActive = true;
    showBlackoutOverlay();
    emit privacyModeChanged(true, "Blackout");
}

void PlayerWindow::activateIntermission()
{
    m_privacyModeActive = true;
    showIntermissionOverlay();
    emit privacyModeChanged(true, "Intermission");
}

void PlayerWindow::deactivatePrivacyMode()
{
    m_privacyModeActive = false;
    hidePrivacyOverlays();
    statusBar()->showMessage("Privacy mode deactivated", 2000);
    emit privacyModeChanged(false);
}

void PlayerWindow::showBlackoutOverlay()
{
    hidePrivacyOverlays(); // Hide any other overlays first
    animatePrivacyTransition(m_blackoutOverlay, true);
}

void PlayerWindow::showIntermissionOverlay()
{
    hidePrivacyOverlays(); // Hide any other overlays first
    animatePrivacyTransition(m_intermissionOverlay, true);
}

void PlayerWindow::hidePrivacyOverlays()
{
    if (m_blackoutOverlay->isVisible()) {
        animatePrivacyTransition(m_blackoutOverlay, false);
    }
    if (m_intermissionOverlay->isVisible()) {
        animatePrivacyTransition(m_intermissionOverlay, false);
    }
}

void PlayerWindow::animatePrivacyTransition(QWidget* overlay, bool show)
{
    if (!overlay) return;

    // Resize overlay to cover entire window
    overlay->resize(size());
    overlay->move(0, 0);

    if (show) {
        overlay->raise(); // Ensure overlay is on top
        // Use AnimationHelper for consistent 200ms fade-in (per CLAUDE.md)
        AnimationHelper::fadeIn(overlay, AnimationHelper::FADE_DURATION);
    } else {
        // Use AnimationHelper for consistent 200ms fade-out
        // The fadeOut helper already handles hiding the widget when animation completes
        AnimationHelper::fadeOut(overlay, AnimationHelper::FADE_DURATION);
    }
}

void PlayerWindow::scheduleRefresh()
{
    if (m_refreshTimer->isActive()) {
        m_refreshTimer->stop();
    }
    m_refreshTimer->start(REFRESH_DEBOUNCE_MS);
}

void PlayerWindow::scheduleAutoFit()
{
    if (m_autoFitTimer->isActive()) {
        m_autoFitTimer->stop();
    }
    m_autoFitTimer->start(AUTOFIT_DEBOUNCE_MS);
}
