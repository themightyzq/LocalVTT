#include "ui/PlayerWindow.h"
#include "graphics/MapDisplay.h"
#include "utils/SettingsManager.h"
#include "utils/AnimationHelper.h"
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
#include <QGuiApplication>
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
    , m_rotation(0)
    , m_blackoutOverlay(nullptr)
    , m_intermissionOverlay(nullptr)
    , m_intermissionLabel(nullptr)
    , m_privacyModeActive(false)
    , m_tripleClickTimer(new QTimer(this))
    , m_clickCount(0)
{
    // Set object name for fog rendering detection
    setObjectName("PlayerWindow");

    // Set window flags for better window management
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

    // CLAUDE.md COMPLIANCE: Player window has ZERO UI - Pure immersion
    // Hide all UI elements for complete immersion
    menuBar()->hide();  // Remove menu bar completely
    statusBar()->hide(); // Remove status bar completely

    // Create invisible actions for keyboard shortcuts only
    // F11 for fullscreen toggle
    QAction* fullScreenAction = new QAction(this);
    fullScreenAction->setShortcut(QKeySequence("F11"));
    connect(fullScreenAction, &QAction::triggered, [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });
    addAction(fullScreenAction);  // Add to window without menu

    // Ctrl+Shift+F for auto-fit toggle (invisible)
    QAction* autoFitAction = new QAction(this);
    autoFitAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(autoFitAction, &QAction::triggered, [this]() {
        m_autoFitEnabled = !m_autoFitEnabled;
        if (m_autoFitEnabled) {
            autoFitToScreen();
        }
    });
    addAction(autoFitAction);  // Add to window without menu
    
    // Create privacy overlays
    createPrivacyOverlays();

    // Setup triple-click detection timer
    m_tripleClickTimer->setSingleShot(true);
    connect(m_tripleClickTimer, &QTimer::timeout, [this]() {
        m_clickCount = 0; // Reset count after timeout
    });

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

void PlayerWindow::updateDisplay()
{
    // Ensure the player view is sharing the latest scene and items
    if (!m_playerView) return;

    // Re-share to sync pointers to map and overlays after late loads
    if (m_sharedDisplay) {
        m_playerView->shareScene(m_sharedDisplay);
    }

    // Lightweight refresh
    m_playerView->updateSharedScene();
    m_playerView->update();
    if (m_playerView->scene()) m_playerView->scene()->update();

    // FIX: Reapply rotation after shareScene() reset the transform
    // shareScene() calls resetTransform() + scale() which clears any rotation
    if (m_rotation != 0) {
        if (m_autoFitEnabled) {
            autoFitToScreen();  // This already handles rotation
        } else {
            m_playerView->rotate(m_rotation);
        }
    }
}

void PlayerWindow::forceRefresh()
{
    // Force immediate scene refresh - calls updateDisplay() plus viewport repaint
    updateDisplay();
    if (m_playerView && m_playerView->viewport()) {
        m_playerView->viewport()->repaint();
    }
}

void PlayerWindow::setEnhancedStyling()
{
    // Premium minimalist styling - let ThemeManager handle most of it
    // Only add window-specific enhancements here
    QString additionalStyle = QString(
        "QMainWindow {"
        "   background: #000000;"
        "}"
        "QGraphicsView {"
        "   background: #000000;"
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

    // Store initial screen and auto-fit if enabled
    m_currentScreen = screen();
    if (m_autoFitEnabled) {
        // Delay to ensure window is fully shown
        QTimer::singleShot(500, this, &PlayerWindow::autoFitToScreen);
    }

    // Ensure player view is synced with the latest scene when the window becomes visible
    updateDisplay();
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
    // Only sync zoom if auto-fit is disabled
    // When auto-fit is enabled, Player window maintains its own zoom
    if (m_playerView && !m_autoFitEnabled) {
        m_playerView->syncZoomLevel(zoomLevel, centerPoint);
        // Reapply rotation after zoom sync (syncZoomLevel resets transform)
        if (m_rotation != 0) {
            m_playerView->rotate(m_rotation);
        }
    }
}

void PlayerWindow::fitToView()
{
    // When called from DM window, only fit if auto-fit is disabled
    // Otherwise Player window manages its own display
    if (m_playerView && !m_autoFitEnabled) {
        m_playerView->fitMapToView();
        // Reapply rotation after fit (fitMapToView resets transform)
        if (m_rotation != 0) {
            m_playerView->rotate(m_rotation);
        }
    }
}

void PlayerWindow::syncViewFromDM(qreal zoomLevel, const QPointF& centerPoint)
{
    // DM explicitly wants to show players a specific view
    // Disable auto-fit and sync to DM's zoom/pan
    m_autoFitEnabled = false;

    if (m_playerView) {
        m_playerView->syncZoomLevel(zoomLevel, centerPoint);
        // Reapply rotation after zoom sync (syncZoomLevel resets transform)
        if (m_rotation != 0) {
            m_playerView->rotate(m_rotation);
        }
    }
}

void PlayerWindow::resetToAutoFit()
{
    // Return to auto-fit mode
    m_autoFitEnabled = true;
    autoFitToScreen();
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
}

void PlayerWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    SettingsManager::instance().saveWindowGeometry("PlayerWindow", geometry());

    // Auto-fit when window is resized (e.g., maximized on TV)
    if (m_autoFitEnabled) {
        // Delay slightly to let resize complete
        QTimer::singleShot(100, this, &PlayerWindow::autoFitToScreen);
    }
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
            // Window moved to different monitor - auto-fit the map (silently)
            // CLAUDE.md COMPLIANCE: No UI feedback, pure immersion
            QTimer::singleShot(200, this, &PlayerWindow::autoFitToScreen);
        }
    }
}

void PlayerWindow::autoFitToScreen()
{
    if (m_playerView && m_playerView->scene() && !m_playerView->scene()->items().isEmpty()) {
        // Get the first pixmap item (the map)
        QList<QGraphicsItem*> items = m_playerView->scene()->items();
        QGraphicsPixmapItem* mapItem = nullptr;
        for (QGraphicsItem* item : items) {
            if (auto* pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(item)) {
                mapItem = pixmapItem;
                break;
            }
        }

        if (mapItem) {
            // Fit the entire map in view without cropping
            // Better to have small black bars than to lose parts of the map
            m_playerView->fitInView(mapItem, Qt::KeepAspectRatio);

            // Reapply rotation after fitInView (fitInView resets the transform)
            if (m_rotation != 0) {
                m_playerView->rotate(m_rotation);
            }
        }

        // CLAUDE.md COMPLIANCE: Auto-fit silently without any UI feedback
        // Pure immersion - no status messages
    }
}

void PlayerWindow::setRotation(int rotation)
{
    m_rotation = rotation % 360;
    // Rotation will be applied on next autoFitToScreen() call
    // or immediately if we have a scene
    if (m_playerView && m_playerView->scene()) {
        QRectF sceneRect = m_playerView->scene()->sceneRect();
        if (!sceneRect.isEmpty()) {
            m_playerView->fitInView(sceneRect, Qt::KeepAspectRatio);
            if (m_rotation != 0) {
                m_playerView->rotate(m_rotation);
            }
        }
    }
}

void PlayerWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: #2D2D2D; color: #E0E0E0; border: 1px solid #3A3A3A; padding: 4px; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #4A90E2; }"
        "QMenu::separator { height: 1px; background: #3A3A3A; margin: 4px 8px; }"
    );

    // Display management
    QMenu* displayMenu = menu.addMenu("Send to Display");
    displayMenu->setStyleSheet(menu.styleSheet());
    for (QScreen* screen : QGuiApplication::screens()) {
        QString label = screen->name() + QString(" (%1x%2)").arg(screen->size().width()).arg(screen->size().height());
        QAction* action = displayMenu->addAction(label);
        connect(action, &QAction::triggered, this, [this, screen]() {
            moveToScreen(screen, true);
        });
    }

    menu.addSeparator();

    // View controls
    QAction* fullscreenAction = menu.addAction(isFullScreen() ? "Exit Fullscreen" : "Fullscreen (F11)");
    connect(fullscreenAction, &QAction::triggered, this, [this]() {
        isFullScreen() ? showNormal() : showFullScreen();
    });

    QAction* fitAction = menu.addAction("Reset Auto-Fit");
    connect(fitAction, &QAction::triggered, this, &PlayerWindow::resetToAutoFit);

    menu.addSeparator();

    // Privacy modes
    QAction* blackoutAction = menu.addAction("Blackout");
    connect(blackoutAction, &QAction::triggered, this, &PlayerWindow::activateBlackout);

    QAction* intermissionAction = menu.addAction("Intermission");
    connect(intermissionAction, &QAction::triggered, this, &PlayerWindow::activateIntermission);

    if (m_privacyModeActive) {
        menu.addSeparator();
        QAction* exitPrivacyAction = menu.addAction("Exit Privacy Mode (Esc)");
        connect(exitPrivacyAction, &QAction::triggered, this, &PlayerWindow::deactivatePrivacyMode);
    }

    menu.exec(event->globalPos());
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
            // CLAUDE.md COMPLIANCE: No status messages for pure immersion
            return;
        }
    }

    // Handle F11 to toggle fullscreen
    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) {
            showNormal();
            // CLAUDE.md COMPLIANCE: No status messages for pure immersion
        } else {
            showFullScreen();
            // CLAUDE.md COMPLIANCE: No status messages for pure immersion
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
    // CLAUDE.md COMPLIANCE: No status messages for pure immersion
    // Triple-click blackout is silent - users know Escape exits
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
    // CLAUDE.md COMPLIANCE: No status messages for pure immersion
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

// ============================================================
// Multi-Monitor Support
// ============================================================

QList<QScreen*> PlayerWindow::getAvailableScreens()
{
    return QGuiApplication::screens();
}

QScreen* PlayerWindow::findSecondaryScreen()
{
    QScreen* primary = QGuiApplication::primaryScreen();
    QList<QScreen*> screens = QGuiApplication::screens();

    // Find any screen that is not the primary screen
    for (QScreen* screen : screens) {
        if (screen != primary) {
            return screen;
        }
    }

    return nullptr;  // No secondary screen found
}

void PlayerWindow::moveToScreen(QScreen* screen, bool fullscreen)
{
    if (!screen) return;

    // Get the geometry of the target screen
    QRect screenGeometry = screen->geometry();

    // If currently fullscreen, exit first to allow proper repositioning
    if (isFullScreen()) {
        showNormal();
    }

    // Move window to the target screen
    // First set geometry to position on that screen
    setGeometry(screenGeometry);

    // Update our tracked screen
    m_currentScreen = screen;

    // Go fullscreen if requested
    if (fullscreen) {
        // Small delay to ensure window is positioned before going fullscreen
        QTimer::singleShot(100, this, [this]() {
            showFullScreen();
            // Auto-fit after going fullscreen
            QTimer::singleShot(200, this, &PlayerWindow::autoFitToScreen);
        });
    } else {
        // Just maximize on the target screen
        showMaximized();
        QTimer::singleShot(100, this, &PlayerWindow::autoFitToScreen);
    }
}

void PlayerWindow::moveToSecondaryDisplay()
{
    QScreen* secondary = findSecondaryScreen();
    if (secondary) {
        moveToScreen(secondary, true);  // Move to secondary and go fullscreen
    }
}
