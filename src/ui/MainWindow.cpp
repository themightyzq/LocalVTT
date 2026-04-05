#include "ui/MainWindow.h"
#include "graphics/MapDisplay.h"
#include "utils/ActionRegistry.h"
#include "utils/AnimationHelper.h"
#include "ui/PlayerWindow.h"
#include "graphics/FogOfWar.h"
#include "graphics/GridOverlay.h"
#include "utils/SettingsManager.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/dialogs/LightEditDialog.h"
#include "ui/ToolboxWidget.h"
#include "ui/ToastNotification.h"
#include "ui/LoadingOverlay.h"
#include "ui/KeyboardShortcutsOverlay.h"
#include "ui/DarkTheme.h"
#include "ui/DebugConsoleWidget.h"
#include "utils/DebugConsole.h"
#include "utils/ErrorHandler.h"
#include "controllers/RecentFilesController.h"
#include "controllers/FogAutosaveController.h"
#include "controllers/TabsController.h"
#include "controllers/FogToolsController.h"
#include "controllers/GridController.h"
#include "controllers/LightingController.h"
#include "controllers/ViewZoomController.h"
#include "controllers/PlayerWindowController.h"
#include "controllers/AtmosphereController.h"
#include "controllers/AtmosphereManager.h"
#include "controllers/ToolManager.h"
#include "utils/ToolType.h"
#include "ui/widgets/ToolStatusWidget.h"
#include "ui/widgets/MapBrowserWidget.h"
#include "ui/AtmosphereToolboxWidget.h"
#include "utils/MapSession.h"
#include "graphics/ToolOverlayWidget.h"
#include "graphics/LightingOverlay.h"
#include "graphics/PointLightSystem.h"
#include "graphics/GMBeacon.h"
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QAction>
#include <QActionGroup>
#include <QStatusBar>
#include <QProgressDialog>
#include <QDragEnterEvent>
#include <QCloseEvent>
#include <QMimeData>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QStyle>
#include <QStyleOption>
#include <QThread>
#include <QUrl>
#include <QStandardPaths>
#include <QLabel>
#include <QHBoxLayout>
#include <QDir>
#include <QKeyEvent>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QSpinBox>
#include <QButtonGroup>
#include <QPointer>

// Define static constants
const int MainWindow::MaxRecentFiles;
const int MainWindow::MAX_TABS;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mapDisplay(nullptr)
    , m_playerWindow(nullptr)
    , m_toolboxWidget(nullptr)
    , m_actionRegistry(nullptr)
    , m_menuManager(nullptr)
    , m_fileOperationsManager(nullptr)
    , m_toolManager(nullptr)
    , m_gridController(nullptr)
    , m_lightingController(nullptr)
    , m_viewZoomController(nullptr)
    , m_playerWindowController(nullptr)
    , m_atmosphereController(nullptr)
    , m_tabBar(nullptr)
    , m_recentFilesMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_sendToDisplayMenu(nullptr)
    , m_zoomToolBar(nullptr)
    , m_clearRecentAction(nullptr)
    , m_recentFilesController(nullptr)
    , m_tabsController(nullptr)
    , m_progressDialog(nullptr)
    , m_loadingOverlay(nullptr)
    , m_dropOverlay(nullptr)
    , m_dropAnimation(nullptr)
    , m_fogLocked(false)
    , m_gridLocked(false)
    , m_mapRotation(0)
    , m_playerRotation(0)
    , m_syncRotationToPlayer(true)  // Default: sync rotation to player
    , m_isDragging(false)
    , m_updatingZoomSpinner(false)
    , m_fogToolMode(FogToolMode::UnifiedFog)
    , m_playerViewModeEnabled(false)
    , m_statusContainer(nullptr)
    , m_gridStatusLabel(nullptr)
    , m_fogStatusLabel(nullptr)
    , m_playerViewStatusLabel(nullptr)
    , m_zoomStatusLabel(nullptr)
    , m_privacyStatusLabel(nullptr)
    , m_rotationStatusLabel(nullptr)
    , m_toolStatusWidget(nullptr)
    , m_fitToScreenAction(nullptr)
    , m_zoomInAction(nullptr)
    , m_zoomOutAction(nullptr)
    , m_zoomPercentageLabel(nullptr)
    , m_gridSizeSpinner(nullptr)
    , m_gridSizeLabel(nullptr)
    , m_lockGridAction(nullptr)
    , m_zoomSpinner(nullptr)
    , m_lockFogAction(nullptr)
    , m_fogBrushSlider(nullptr)
    , m_fogBrushLabel(nullptr)
    , m_gmOpacitySlider(nullptr)
    , m_gmOpacityLabel(nullptr)
    , m_fogToolsController(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_toggleGridAction(nullptr)
    , m_playerWindowToggleAction(nullptr)
    , m_syncViewToPlayersAction(nullptr)
    , m_resetPlayerAutoFitAction(nullptr)
    , m_syncRotationToggleAction(nullptr)
    , m_syncRotationNowAction(nullptr)
    , m_rotatePlayerViewAction(nullptr)
    , m_fogHideToggleAction(nullptr)
    , m_fogBrushAction(nullptr)
    , m_resetFogAction(nullptr)
    , m_brushSizeDebounceTimer(nullptr)
    , m_gridSizeDebounceTimer(nullptr)
    , m_controlStripDock(nullptr)
    , m_lightingAction(nullptr)
    , m_timeOfDayButtonGroup(nullptr)
    , m_dawnAction(nullptr)
    , m_dayAction(nullptr)
    , m_duskAction(nullptr)
    , m_nightAction(nullptr)
    , m_lightingIntensitySlider(nullptr)
    , m_lightingIntensityLabel(nullptr)
    , m_timeOfDayLabel(nullptr)
    , m_pointLightPlacementAction(nullptr)
    , m_ambientLightSlider(nullptr)
    , m_ambientLightLabel(nullptr)
    , m_clearPointLightsAction(nullptr)
    , m_exposureSlider(nullptr)
    , m_exposureLabel(nullptr)
    , m_fogAutosaveController(nullptr)
    , m_debugConsoleWidget(nullptr)
    , m_debugConsoleAction(nullptr)
    , m_shortcutsOverlay(nullptr)
    , m_mapBrowserWidget(nullptr)
    , m_mapBrowserAction(nullptr)
    , m_atmosphereToolbox(nullptr)
    , m_atmosphereToolboxAction(nullptr)
{

    // CRITICAL: Remove try-catch blocks that hide errors - we need to see what's failing
    // Set initial window properties
    setWindowTitle("Crit VTT");
    resize(1200, 800);
    setMinimumSize(960, 600);

    // Initialize ActionRegistry first (many components depend on it)
    m_actionRegistry = new ActionRegistry(this);

    // Load settings and window geometry
    SettingsManager& settings = SettingsManager::instance();
    QRect defaultGeometry(100, 100, 1200, 800);
    QRect savedGeometry = settings.loadWindowGeometry("MainWindow", defaultGeometry);
    setGeometry(savedGeometry);

    // MEMORY OPTIMIZATION: Defer heavy UI initialization
    // Only create bare minimum UI components initially
    setupMinimalUI();  // Create only essential components

    // Defer creation of controllers and managers until needed
    QPointer<MainWindow> self = this;
    QTimer::singleShot(10, this, [self]() {
        if (self) self->setupDeferredComponents();
    });

    setupActions();

    createMenus();

    // MEMORY OPTIMIZATION: Defer toolbox and fog controller creation

    // Setup fog tool mode system BEFORE creating toolbar (toolbar needs these actions)
    setupFogToolModeSystem();

    createToolbar();

    createZoomToolbar();

    setupStatusBar();

    // Initialize controllers that depend on UI components
    // Note: ToolManager and FogToolsController will be created after MapDisplay
    // is created in loadMapFile() to avoid null pointer issues

    // MEMORY OPTIMIZATION: Defer controller creation until needed
    // Only create essential controllers immediately
    m_recentFilesController = new RecentFilesController(this);

    // These will be created on-demand:
    // m_tabsController - created in loadMapFile
    // m_gridController - created when grid is first used
    // m_lightingController - created when lighting is first used
    // m_viewZoomController - created when zoom is first used
    // m_playerWindowController - created when player window is opened
    // m_fogAutosaveController - created when fog is first enabled
    // m_fogToolsController - created when fog tools are first used

    // Update UI states
    updateRecentFilesMenu();
    updateUndoRedoButtons();
    updateAllMenuStates();

    // Connect to ErrorHandler for auto-blackout on critical errors
    connect(&ErrorHandler::instance(), &ErrorHandler::errorOccurred,
            this, [this](const QString& message, ErrorLevel level) {
        if (level == ErrorLevel::Critical) {
            // Auto-blackout on critical errors to protect player screen
            activateBlackout();
            statusBar()->showMessage("Critical error detected - Player screen protected", 10000);
        }
    });

    // Final setup
    setObjectName("MainWindow");

    setAcceptDrops(true);
    setWindowTitle("Crit VTT");

    // CRITICAL: Set focus policy so MainWindow can receive keyboard events
    // This is required for shortcuts to work without clicking inside first
    setFocusPolicy(Qt::StrongFocus);
}

MainWindow::~MainWindow()
{
    // Save window geometry before shutdown
    SettingsManager::instance().saveWindowGeometry("MainWindow", geometry());

    // Clean shutdown - stop animations first
    if (m_dropAnimation) {
        m_dropAnimation->stop();
        delete m_dropAnimation;
    }

    // TabsController owns sessions; nothing to do here.

    if (m_playerWindow) {
        delete m_playerWindow;
    }
    if (m_progressDialog) {
        delete m_progressDialog;
    }
    if (m_dropOverlay) {
        delete m_dropOverlay;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Show confirmation dialog before closing
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Quit Crit VTT?",
        "Are you sure you want to quit?\n\nAny unsaved fog changes will be lost.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No  // Default to No for safety
    );

    if (reply != QMessageBox::Yes) {
        event->ignore();
        return;
    }

    // User confirmed - close Player Window and quit
    if (m_playerWindow) {
        m_playerWindow->close();
    }

    // Save window geometry
    SettingsManager::instance().saveWindowGeometry("MainWindow", geometry());

    // Accept the close event and quit the application
    event->accept();
    QApplication::quit();
}

void MainWindow::setupMinimalUI()
{
    // CRITICAL FIX: Create MapDisplay and ToolManager eagerly to enable tool switching

    // CRITICAL: Remove try-catch blocks to see actual errors
    // Create central widget with tab system
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // CRITICAL FIX: Create MapDisplay and tab widget eagerly
    // This ensures tool switching works immediately, even without a loaded map

    // Create MapDisplay immediately
    m_mapDisplay = new MapDisplay(this);
    m_mapDisplay->setMainWindow(this);

    // Create tab bar (just the tab buttons, not a full tab widget)
    // This allows us to have one shared MapDisplay for all tabs
    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->hide();  // Hide until we have maps

    // Layout: tab bar on top, map display below
    layout->addWidget(m_tabBar);
    layout->addWidget(m_mapDisplay, 1);  // stretch factor 1 to fill space

    // Create controllers early (needed for toolbar creation)
    m_fogToolsController = new FogToolsController(this);
    m_viewZoomController = new ViewZoomController(this);
    m_viewZoomController->setMapDisplay(m_mapDisplay);

    // Create ToolManager immediately so tool switching is available
    m_toolManager = new ToolManager(this, m_mapDisplay, this);

    // Connect tool switching signal from MapDisplay to ToolManager
    connect(m_mapDisplay, &MapDisplay::toolSwitchRequested,
            m_toolManager, &ToolManager::setActiveTool);

    // Connect point light edit signal to show properties dialog
    connect(m_mapDisplay, &MapDisplay::pointLightDoubleClicked,
            this, &MainWindow::showPointLightProperties);

    // Connect tool changed signal to show status messages
    connect(m_toolManager, &ToolManager::toolChanged,
            this, [this](ToolType tool) {
                QString toolName = m_toolManager->getActiveToolName();
                statusBar()->showMessage(QString("Active tool: %1").arg(toolName), 2000);
            });

    // Connect tool changed signal to MapDisplay
    connect(m_toolManager, &ToolManager::toolChanged,
            m_mapDisplay, &MapDisplay::onToolChanged);

    // Connect tool changed signal to update fog button state
    connect(m_toolManager, &ToolManager::toolChanged,
            this, [this](ToolType tool) {
                if (m_fogBrushAction) {
                    m_fogBrushAction->setChecked(tool == ToolType::FogBrush);
                }
            });


    setCentralWidget(centralWidget);

    // Skip signal connections and complex initialization for now
    // TODO: Connect zoom signals later
    // TODO: Setup fog autosave controller later

    // Create drop overlay for visual feedback (hidden by default)
    m_dropOverlay = new QWidget(this);
    m_dropOverlay->setObjectName("dropOverlay");
    m_dropOverlay->hide();
    m_dropOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);

    // No opacity effects or animations - they cause crashes with shared scenes
    m_dropAnimation = nullptr;

}

void MainWindow::setupDeferredComponents()
{

    // These components can be created after initial window is shown
    // They're not needed immediately for basic functionality

    // Defer controller creation - create on first use
    // m_gridController - create when grid is first toggled
    // m_lightingController - create when lighting is first enabled
    // m_viewZoomController - create when zoom is first used
    // m_playerWindowController - create when player window is opened
    // m_fogToolsController - create when fog is first enabled

}

void MainWindow::setupActions()
{
    if (!m_actionRegistry) {
        return;
    }

    // File menu actions
    // CRITICAL: Check for null actions before connecting to prevent segfault
    if (auto* action = getOrCreateAction("file_open")) connect(action, &QAction::triggered, this, &MainWindow::loadMap);
    if (auto* action = getOrCreateAction("file_save")) connect(action, &QAction::triggered, this, &MainWindow::quickSaveFogState);
    if (auto* action = getOrCreateAction("file_load")) connect(action, &QAction::triggered, this, &MainWindow::quickRestoreFogState);
    if (auto* action = getOrCreateAction("file_quit")) connect(action, &QAction::triggered, []() { QApplication::quit(); });

    // Edit menu actions
    m_undoAction = getOrCreateAction("edit_undo");
    m_redoAction = getOrCreateAction("edit_redo");
    if (m_undoAction) connect(m_undoAction, &QAction::triggered, this, &MainWindow::undoFogChange);
    if (m_redoAction) connect(m_redoAction, &QAction::triggered, this, &MainWindow::redoFogChange);
    // Note: Removed edit_preferences - not in simplified action set

    // View menu actions
    m_fitToScreenAction = getOrCreateAction("view_fit_screen");
    m_zoomInAction = getOrCreateAction("view_zoom_in");
    m_zoomOutAction = getOrCreateAction("view_zoom_out");
    if (m_fitToScreenAction) connect(m_fitToScreenAction, &QAction::triggered, this, &MainWindow::fitToScreen);
    if (m_zoomInAction) connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    if (m_zoomOutAction) connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    // Grid actions
    m_toggleGridAction = getOrCreateAction("grid_toggle");
    if (m_toggleGridAction) connect(m_toggleGridAction, &QAction::triggered, this, &MainWindow::toggleGrid);
    if (auto* action = getOrCreateAction("grid_info")) connect(action, &QAction::triggered, this, &MainWindow::showGridInfo);
    if (auto* action = getOrCreateAction("grid_type")) connect(action, &QAction::triggered, this, &MainWindow::toggleGridType);
    // Note: grid_standard removed from simplified action set

    // Fog actions
    if (auto* action = getOrCreateAction("fog_toggle")) connect(action, &QAction::triggered, this, &MainWindow::toggleFogOfWar);
    if (auto* action = getOrCreateAction("fog_clear")) connect(action, &QAction::triggered, this, &MainWindow::clearFogOfWar);
    if (auto* action = getOrCreateAction("fog_reset")) connect(action, &QAction::triggered, this, &MainWindow::resetFogOfWar);
    if (auto* action = getOrCreateAction("fog_hide_toggle")) connect(action, &QAction::triggered, this, &MainWindow::toggleFogHideMode);

    // Window actions - Player Window action is created in createMenus() with correct 'P' shortcut
    // (ActionRegistry has Ctrl+W which is wrong per CLAUDE.md 3.6)

    // Lighting actions (basic toggle only in simplified set)
    m_lightingAction = getOrCreateAction("lighting_toggle");
    if (m_lightingAction) connect(m_lightingAction, &QAction::triggered, this, &MainWindow::toggleLighting);

    // Time of day and point lights are part of weather/effects (sacred features)
    // Create them manually since they're not in ActionRegistry
    m_dawnAction = new QAction("Dawn", this);
    m_dayAction = new QAction("Day", this);
    m_duskAction = new QAction("Dusk", this);
    m_nightAction = new QAction("Night", this);

    m_dawnAction->setCheckable(true);
    m_dayAction->setCheckable(true);
    m_duskAction->setCheckable(true);
    m_nightAction->setCheckable(true);

    connect(m_dawnAction, &QAction::triggered, [this]() { setTimeOfDay(0); });
    connect(m_dayAction, &QAction::triggered, [this]() { setTimeOfDay(1); });
    connect(m_duskAction, &QAction::triggered, [this]() { setTimeOfDay(2); });
    connect(m_nightAction, &QAction::triggered, [this]() { setTimeOfDay(3); });

    m_pointLightPlacementAction = new QAction("Place Point Light", this);
    m_pointLightPlacementAction->setCheckable(true);
    connect(m_pointLightPlacementAction, &QAction::triggered, this, &MainWindow::togglePointLightPlacement);

    m_clearPointLightsAction = new QAction("Clear All Lights", this);
    connect(m_clearPointLightsAction, &QAction::triggered, this, &MainWindow::clearAllPointLights);



    // Debug actions
    m_debugConsoleAction = getOrCreateAction("debug_console");
    if (m_debugConsoleAction) connect(m_debugConsoleAction, &QAction::triggered, this, &MainWindow::toggleDebugConsole);

    // Help actions
    if (auto* action = getOrCreateAction("help_shortcuts")) connect(action, &QAction::triggered, this, &MainWindow::showKeyboardShortcuts);
    // Note: help_guide removed from simplified action set
    if (auto* action = getOrCreateAction("help_about")) connect(action, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

QAction* MainWindow::getOrCreateAction(const QString& actionId)
{
    if (!m_actionRegistry) {
        return nullptr;
    }
    
    QAction* action = m_actionRegistry->getAction(actionId);
    if (!action) {
        action = m_actionRegistry->createAction(actionId, this);
    }
    return action;
}

void MainWindow::createMenus()
{
    // Simplified menu bar with only essential items
    menuBar()->setStyleSheet(R"(
        QMenuBar {
            background: #242424;
            border-bottom: 1px solid #1a1a1a;
            padding: 4px;
        }
        QMenuBar::item {
            padding: 6px 12px;
            color: #E0E0E0;
            background: transparent;
            border-radius: 4px;
            margin: 0 2px;
        }
        QMenuBar::item:selected {
            background: rgba(74, 144, 226, 0.2);
        }
        QMenu {
            background: #2a2a2a;
            border: 1px solid #3a3a3a;
            padding: 4px;
        }
        QMenu::item {
            padding: 8px 24px;
            color: #E0E0E0;
            border-radius: 4px;
            margin: 2px;
        }
        QMenu::item:selected {
            background: rgba(74, 144, 226, 0.25);
        }
        QMenu::separator {
            background: #3a3a3a;
            height: 1px;
            margin: 4px 8px;
        }
    )");

    // FILE MENU - Minimal
    m_fileMenu = menuBar()->addMenu("&File");

    QAction* openAction = new QAction("&Open Map...", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::loadMap);
    m_fileMenu->addAction(openAction);

    // Recent Files
    m_recentFilesMenu = m_fileMenu->addMenu("&Recent Files");
    for (int i = 0; i < MaxRecentFiles; ++i) {
        QAction* action = new QAction(this);
        action->setVisible(false);
        m_recentFileActions.append(action);
        m_recentFilesMenu->addAction(action);
    }
    m_recentFilesMenu->addSeparator();
    m_clearRecentAction = new QAction("Clear Recent", this);
    m_recentFilesMenu->addAction(m_clearRecentAction);

    m_recentFilesController = new RecentFilesController(this);
    m_recentFilesController->attach(m_recentFilesMenu, m_recentFileActions, m_clearRecentAction, MaxRecentFiles);
    connect(m_recentFilesController, &RecentFilesController::openFileRequested,
            this, &MainWindow::loadMapFile);

    m_fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    m_fileMenu->addAction(exitAction);

    // No Edit menu - Undo/Redo available via keyboard shortcuts only

    // VIEW MENU - Simplified
    m_viewMenu = menuBar()->addMenu("&View");

    // Grid toggle - ApplicationShortcut so it works regardless of focus
    QAction* gridToggleAction = new QAction("Toggle &Grid", this);
    gridToggleAction->setShortcut(QKeySequence("G"));
    gridToggleAction->setShortcutContext(Qt::ApplicationShortcut);
    gridToggleAction->setCheckable(true);
    gridToggleAction->setChecked(m_mapDisplay->isGridEnabled());
    connect(gridToggleAction, &QAction::triggered, this, &MainWindow::toggleGrid);
    m_viewMenu->addAction(gridToggleAction);
    m_toggleGridAction = gridToggleAction;

    // Fog toggle - ApplicationShortcut so it works regardless of focus
    QAction* fogToggleAction = getOrCreateAction("fog_toggle");
    if (fogToggleAction) {
        fogToggleAction->setShortcutContext(Qt::ApplicationShortcut);
        fogToggleAction->setChecked(m_mapDisplay->isFogEnabled());
        m_viewMenu->addAction(fogToggleAction);
    }

    // Rectangle fog tool toggle - ApplicationShortcut so it works regardless of focus
    QAction* fogRectangleAction = new QAction("&Rectangle Fog Tool", this);
    fogRectangleAction->setShortcut(QKeySequence("R"));
    fogRectangleAction->setShortcutContext(Qt::ApplicationShortcut);
    fogRectangleAction->setCheckable(true);
    fogRectangleAction->setChecked(m_mapDisplay->isFogRectangleModeEnabled());
    connect(fogRectangleAction, &QAction::triggered, this, &MainWindow::toggleFogRectangleMode);
    m_viewMenu->addAction(fogRectangleAction);
    m_fogRectangleModeAction = fogRectangleAction;

    m_viewMenu->addSeparator();

    // Player Window - store in member for reuse in toolbar
    // ApplicationShortcut so it works regardless of focus
    m_playerWindowToggleAction = new QAction("&Player Window", this);
    m_playerWindowToggleAction->setShortcut(QKeySequence("P"));
    m_playerWindowToggleAction->setShortcutContext(Qt::ApplicationShortcut);
    m_playerWindowToggleAction->setCheckable(true);
    connect(m_playerWindowToggleAction, &QAction::triggered, this, &MainWindow::togglePlayerWindow);
    m_viewMenu->addAction(m_playerWindowToggleAction);

    // Send to Display submenu - multi-monitor support
    m_sendToDisplayMenu = m_viewMenu->addMenu("Send Player to &Display");
    // Menu will be populated dynamically when opened
    connect(m_sendToDisplayMenu, &QMenu::aboutToShow, this, &MainWindow::updateSendToDisplayMenu);

    // View sync controls - push DM view to players
    m_syncViewToPlayersAction = new QAction("S&ync View to Players", this);
    m_syncViewToPlayersAction->setShortcut(QKeySequence("V"));
    m_syncViewToPlayersAction->setShortcutContext(Qt::ApplicationShortcut);
    m_syncViewToPlayersAction->setToolTip("Push your current view to the Player Window (V)");
    connect(m_syncViewToPlayersAction, &QAction::triggered, this, &MainWindow::syncViewToPlayers);
    m_viewMenu->addAction(m_syncViewToPlayersAction);

    m_resetPlayerAutoFitAction = new QAction("Reset Player &Auto-Fit", this);
    m_resetPlayerAutoFitAction->setShortcut(QKeySequence("Shift+V"));
    m_resetPlayerAutoFitAction->setShortcutContext(Qt::ApplicationShortcut);
    m_resetPlayerAutoFitAction->setToolTip("Return Player Window to auto-fit mode (Shift+V)");
    connect(m_resetPlayerAutoFitAction, &QAction::triggered, this, &MainWindow::resetPlayerAutoFit);
    m_viewMenu->addAction(m_resetPlayerAutoFitAction);

    // Rotation sync controls
    m_syncRotationToggleAction = new QAction("Sync &Rotation to Players", this);
    m_syncRotationToggleAction->setCheckable(true);
    m_syncRotationToggleAction->setChecked(m_syncRotationToPlayer);
    m_syncRotationToggleAction->setToolTip("When enabled, DM rotation syncs to Player Window automatically");
    connect(m_syncRotationToggleAction, &QAction::triggered, this, &MainWindow::toggleRotationSync);
    m_viewMenu->addAction(m_syncRotationToggleAction);

    m_syncRotationNowAction = new QAction("Push Rotation to Players", this);
    m_syncRotationNowAction->setShortcut(QKeySequence("Shift+R"));
    m_syncRotationNowAction->setShortcutContext(Qt::ApplicationShortcut);
    m_syncRotationNowAction->setToolTip("Manually sync DM rotation to Player Window (Shift+R)");
    connect(m_syncRotationNowAction, &QAction::triggered, this, &MainWindow::syncRotationToPlayer);
    m_viewMenu->addAction(m_syncRotationNowAction);

    m_rotatePlayerViewAction = new QAction("Rotate Player View", this);
    m_rotatePlayerViewAction->setShortcut(QKeySequence("Ctrl+R"));
    m_rotatePlayerViewAction->setShortcutContext(Qt::ApplicationShortcut);
    m_rotatePlayerViewAction->setToolTip("Rotate only the Player Window view 90° clockwise (Ctrl+R)");
    connect(m_rotatePlayerViewAction, &QAction::triggered, this, &MainWindow::rotatePlayerView);
    m_viewMenu->addAction(m_rotatePlayerViewAction);

    m_viewMenu->addSeparator();

    // Zoom controls
    // Fit to Screen - reuse MenuManager's action via m_fitToScreenAction if available,
    // otherwise create without shortcut (shortcut handled by MenuManager)
    QAction* fitAction = new QAction("&Fit to Screen", this);
    // No shortcut here - MenuManager sets the authoritative shortcut
    connect(fitAction, &QAction::triggered, this, &MainWindow::fitToScreen);
    m_viewMenu->addAction(fitAction);

    QAction* zoomInAction = new QAction("Zoom &In", this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    m_viewMenu->addAction(zoomInAction);

    QAction* zoomOutAction = new QAction("Zoom &Out", this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    m_viewMenu->addAction(zoomOutAction);

    m_viewMenu->addSeparator();

    // Atmosphere submenu - for lighting/weather transitions
    m_atmosphereController = new AtmosphereController(this);
    m_atmosphereController->attachToMainWindow(this);
    if (m_mapDisplay) {
        m_atmosphereController->setMapDisplay(m_mapDisplay);
    }
    QMenu* atmosphereMenu = m_atmosphereController->createAtmosphereMenu(m_viewMenu);
    m_viewMenu->addMenu(atmosphereMenu);

    m_viewMenu->addSeparator();

    // Fog Controls submenu - for GM opacity and brush size presets
    QMenu* fogControlsMenu = m_viewMenu->addMenu("Fog &Controls");

    // GM Opacity presets
    QAction* opacity25Action = new QAction("DM Opacity: &25%", this);
    connect(opacity25Action, &QAction::triggered, [this]() {
        if (m_mapDisplay && m_mapDisplay->getFogOverlay()) {
            m_mapDisplay->getFogOverlay()->setGMOpacity(0.25);
            if (m_toolboxWidget) m_toolboxWidget->updateGMOpacity(25);
        }
    });
    fogControlsMenu->addAction(opacity25Action);

    QAction* opacity50Action = new QAction("DM Opacity: &50%", this);
    connect(opacity50Action, &QAction::triggered, [this]() {
        if (m_mapDisplay && m_mapDisplay->getFogOverlay()) {
            m_mapDisplay->getFogOverlay()->setGMOpacity(0.50);
            if (m_toolboxWidget) m_toolboxWidget->updateGMOpacity(50);
        }
    });
    fogControlsMenu->addAction(opacity50Action);

    QAction* opacity75Action = new QAction("DM Opacity: &75%", this);
    connect(opacity75Action, &QAction::triggered, [this]() {
        if (m_mapDisplay && m_mapDisplay->getFogOverlay()) {
            m_mapDisplay->getFogOverlay()->setGMOpacity(0.75);
            if (m_toolboxWidget) m_toolboxWidget->updateGMOpacity(75);
        }
    });
    fogControlsMenu->addAction(opacity75Action);

    QAction* opacity100Action = new QAction("DM Opacity: &100%", this);
    connect(opacity100Action, &QAction::triggered, [this]() {
        if (m_mapDisplay && m_mapDisplay->getFogOverlay()) {
            m_mapDisplay->getFogOverlay()->setGMOpacity(1.0);
            if (m_toolboxWidget) m_toolboxWidget->updateGMOpacity(100);
        }
    });
    fogControlsMenu->addAction(opacity100Action);

    fogControlsMenu->addSeparator();

    // Brush Size presets
    QAction* brushSmallAction = new QAction("Brush Size: &Small (50px)", this);
    connect(brushSmallAction, &QAction::triggered, [this]() {
        if (m_mapDisplay) {
            m_mapDisplay->setFogBrushSize(50);
            if (m_toolboxWidget) m_toolboxWidget->updateFogBrushSize(50);
        }
    });
    fogControlsMenu->addAction(brushSmallAction);

    QAction* brushMediumAction = new QAction("Brush Size: &Medium (150px)", this);
    connect(brushMediumAction, &QAction::triggered, [this]() {
        if (m_mapDisplay) {
            m_mapDisplay->setFogBrushSize(150);
            if (m_toolboxWidget) m_toolboxWidget->updateFogBrushSize(150);
        }
    });
    fogControlsMenu->addAction(brushMediumAction);

    QAction* brushLargeAction = new QAction("Brush Size: &Large (300px)", this);
    connect(brushLargeAction, &QAction::triggered, [this]() {
        if (m_mapDisplay) {
            m_mapDisplay->setFogBrushSize(300);
            if (m_toolboxWidget) m_toolboxWidget->updateFogBrushSize(300);
        }
    });
    fogControlsMenu->addAction(brushLargeAction);

    // Beacon Color submenu - full color palette
    QMenu* beaconColorMenu = m_viewMenu->addMenu("&Beacon Color");
    QList<QColor> beaconColors = GMBeacon::presetColors();
    // Names match GMBeacon::presetColors() order: Red, Blue, Yellow, Green, Orange, Purple, White, Grey, Black
    QStringList colorNames = {"&Red", "&Blue", "&Yellow", "&Green", "&Orange", "&Purple", "&White", "Gre&y", "Blac&k"};

    for (int i = 0; i < qMin(beaconColors.size(), colorNames.size()); ++i) {
        QAction* colorAction = new QAction(colorNames[i], this);
        QColor color = beaconColors[i];
        connect(colorAction, &QAction::triggered, [this, color]() {
            if (m_mapDisplay) {
                m_mapDisplay->setBeaconColor(color);
            }
            if (m_toolboxWidget) {
                m_toolboxWidget->setBeaconColor(color);
            }
        });
        beaconColorMenu->addAction(colorAction);
    }

    m_viewMenu->addSeparator();

    // Map Browser dock widget
    m_mapBrowserWidget = new MapBrowserWidget(this);
    m_mapBrowserWidget->setRecentFilesController(m_recentFilesController);
    addDockWidget(Qt::RightDockWidgetArea, m_mapBrowserWidget);
    m_mapBrowserWidget->hide();  // Hidden by default

    // Connect map selection to file loading
    connect(m_mapBrowserWidget, &MapBrowserWidget::mapSelected,
            this, &MainWindow::loadMapFile);

    m_mapBrowserAction = new QAction("Map &Browser", this);
    m_mapBrowserAction->setShortcut(QKeySequence("B"));
    m_mapBrowserAction->setCheckable(true);
    m_mapBrowserAction->setChecked(false);
    connect(m_mapBrowserAction, &QAction::triggered, this, &MainWindow::toggleMapBrowser);
    m_viewMenu->addAction(m_mapBrowserAction);

    // Atmosphere Toolbox dock widget
    m_atmosphereToolbox = new AtmosphereToolboxWidget(this);
    if (m_atmosphereController && m_atmosphereController->getAtmosphereManager()) {
        m_atmosphereToolbox->setAtmosphereManager(m_atmosphereController->getAtmosphereManager());
        auto* mgr = m_atmosphereController->getAtmosphereManager();
        m_atmosphereToolbox->setAudioSystems(mgr->getAmbientPlayer(), mgr->getMusicRemote());
    }
    addDockWidget(Qt::RightDockWidgetArea, m_atmosphereToolbox);
    m_atmosphereToolbox->hide();  // Hidden by default

    // Tabify right-side panels so they share space when both open
    tabifyDockWidget(m_mapBrowserWidget, m_atmosphereToolbox);

    m_atmosphereToolboxAction = new QAction("&Atmosphere Panel", this);
    m_atmosphereToolboxAction->setShortcut(QKeySequence("A"));
    m_atmosphereToolboxAction->setCheckable(true);
    m_atmosphereToolboxAction->setChecked(false);
    connect(m_atmosphereToolboxAction, &QAction::triggered, this, &MainWindow::toggleAtmosphereToolbox);
    m_viewMenu->addAction(m_atmosphereToolboxAction);

    // TOOLS MENU - Fog tools (NO keyboard shortcuts per CLAUDE.md)
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");

    QAction* fogToolAction = new QAction("&Fog Brush", this);
    fogToolAction->setCheckable(true);
    connect(fogToolAction, &QAction::triggered, [this]() {
        if (m_toolManager) m_toolManager->setActiveTool(ToolType::FogBrush);
    });
    toolsMenu->addAction(fogToolAction);

    QAction* pointerToolAction = new QAction("&Pointer/Beacon", this);
    pointerToolAction->setCheckable(true);
    connect(pointerToolAction, &QAction::triggered, [this]() {
        if (m_toolManager) m_toolManager->setActiveTool(ToolType::Pointer);
    });
    toolsMenu->addAction(pointerToolAction);

    toolsMenu->addSeparator();

    // Fog commands
    QAction* clearFogAction = new QAction("&Clear All Fog", this);
    connect(clearFogAction, &QAction::triggered, this, &MainWindow::clearFogOfWar);
    toolsMenu->addAction(clearFogAction);

    QAction* resetFogAction = new QAction("&Reset Fog", this);
    connect(resetFogAction, &QAction::triggered, this, &MainWindow::resetFogOfWar);
    toolsMenu->addAction(resetFogAction);

    // HELP MENU - Minimal
    m_helpMenu = menuBar()->addMenu("&Help");

    QAction* shortcutsAction = new QAction("&Keyboard Shortcuts", this);
    connect(shortcutsAction, &QAction::triggered, this, &MainWindow::showKeyboardShortcuts);
    m_helpMenu->addAction(shortcutsAction);

    m_helpMenu->addSeparator();

    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    m_helpMenu->addAction(aboutAction);

    // Debug console hidden behind F12 only
    m_debugConsoleAction = new QAction("Debug Console", this);
    m_debugConsoleAction->setShortcut(QKeySequence("F12"));
    m_debugConsoleAction->setVisible(false);  // Hidden from menus
    connect(m_debugConsoleAction, &QAction::triggered, this, &MainWindow::toggleDebugConsole);
    addAction(m_debugConsoleAction);  // Add as window action for shortcut to work
}

void MainWindow::createToolbar()
{
    m_mainToolBar = addToolBar("Main");
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);  // Icons only to save space

    // Toolbar styling from DarkTheme
    m_mainToolBar->setStyleSheet(DarkTheme::toolBarStyle() + DarkTheme::toolButtonStyle());

    // SECTION 1: File Operations
    // Load Map button with premium styling
    QIcon openIcon(":/icons/open_map.svg");
    QAction* loadAction = new QAction(openIcon, "Load Map", this);
    loadAction->setToolTip("<b>Load Map</b><br>Open a new map image or VTT file<br><i>Shortcut: Ctrl+O</i>");
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadMap);
    m_mainToolBar->addAction(loadAction);

    m_mainToolBar->addSeparator();

    // SECTION 2: Fog Controls (delegated to controller)
    {
        QAction* fogToggleAction = getOrCreateAction("fog_toggle");
        auto fogActions = m_fogToolsController->createToolbarActions(m_mainToolBar, fogToggleAction);
        m_fogHideToggleAction = fogActions.hideToggle;
        m_fogBrushAction = fogActions.brushTool;
        m_fogRectAction = fogActions.rectTool;
        m_resetFogAction = fogActions.resetFog;
        m_lockFogAction = fogActions.lockFog;

        // Wire cross-cutting connections (these reference MainWindow state)
        connect(m_fogHideToggleAction, &QAction::triggered, this, &MainWindow::toggleFogHideMode);
        connect(m_fogBrushAction, &QAction::triggered, this, [this]() {
            if (m_toolManager) {
                m_toolManager->setActiveTool(ToolType::FogBrush);
                statusBar()->showMessage("Reveal Brush active - Click/drag to reveal areas", 3000);
                updateContextualToolbarControls();
            }
        });
        connect(m_fogRectAction, &QAction::triggered, this, [this]() {
            if (m_toolManager) {
                m_toolManager->setActiveTool(ToolType::FogRectangle);
                statusBar()->showMessage("Reveal Rectangle active - Drag to reveal rectangular area", 3000);
                updateContextualToolbarControls();
            }
        });
        connect(m_resetFogAction, &QAction::triggered, this, &MainWindow::resetFogOfWar);
        connect(m_lockFogAction, &QAction::triggered, this, &MainWindow::toggleFogLock);
    }

    m_mainToolBar->addSeparator();

    // SECTION 3: Zoom Controls (delegated to controller)
    m_zoomSpinner = m_viewZoomController->createToolbarActions(m_mainToolBar);
    connect(m_zoomSpinner, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        if (m_updatingZoomSpinner) return;
        if (m_mapDisplay) {
            m_mapDisplay->setZoomLevel(value / 100.0);
        }
    });

    QAction* centerAction = new QAction(QIcon(":/icons/center.svg"), "Center", this);
    centerAction->setToolTip("<b>Center on Map</b><br>Re-center view on map<br>(without changing zoom)");
    connect(centerAction, &QAction::triggered, this, &MainWindow::centerOnMap);
    m_mainToolBar->addAction(centerAction);

    QAction* rotateAction = new QAction(QIcon(":/icons/rotate.svg"), "Rotate", this);
    rotateAction->setToolTip("<b>Rotate Map</b><br>Rotate 90° clockwise");
    connect(rotateAction, &QAction::triggered, this, &MainWindow::rotateMap);
    m_mainToolBar->addAction(rotateAction);

    m_mainToolBar->addSeparator();

    // SECTION 4: Grid Toggle (Simple checkbox)
    // Reuse the menu action to avoid duplicate shortcuts
    if (m_toggleGridAction) {
        m_toggleGridAction->setIcon(QIcon(":/icons/grid.svg"));
        m_toggleGridAction->setText("Grid");
        m_toggleGridAction->setToolTip("<b>Grid Overlay</b><br>Show/hide grid lines<br><i>Shortcut: G</i>");
        // Shortcut already set in menu at line 623 - don't duplicate
        m_mainToolBar->addAction(m_toggleGridAction);
    }

    // Lock Grid Button
    m_lockGridAction = new QAction(QIcon(":/icons/unlock.svg"), "Lock Grid", this);
    m_lockGridAction->setToolTip("<b>Grid Unlocked</b><br>Click to lock grid editing");
    m_lockGridAction->setCheckable(true);
    m_lockGridAction->setChecked(false);
    connect(m_lockGridAction, &QAction::triggered, this, &MainWindow::toggleGridLock);
    m_mainToolBar->addAction(m_lockGridAction);

    m_mainToolBar->addSeparator();

    // SECTION 5: Player Window (Most Important)
    // Reuse the menu action to avoid duplicate shortcuts
    if (m_playerWindowToggleAction) {
        m_playerWindowToggleAction->setIcon(QIcon(":/icons/player_view.svg"));
        m_playerWindowToggleAction->setText("Player View");
        m_playerWindowToggleAction->setChecked(m_playerWindow != nullptr);
        m_playerWindowToggleAction->setToolTip("<b>Player View</b><br>Open TV display window<br>Show map to players<br><i>Shortcut: P</i>");
        // Shortcut already set in menu at line 650 - don't duplicate
        // Connection already established in menu setup - don't duplicate
        m_mainToolBar->addAction(m_playerWindowToggleAction);
    }

    // Make player window button more prominent with animation
    if (auto* btn = qobject_cast<QToolButton*>(m_mainToolBar->widgetForAction(m_playerWindowToggleAction))) {
        btn->setStyleSheet(R"(
            QToolButton {
                background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                            stop: 0 #2e5a9e, stop: 1 #254a8e);
                border: 1px solid #4A90E2;
                border-radius: 6px;
                padding: 10px 20px;
                color: white;
                font-size: 14px;
                font-weight: 600;
                min-width: 120px;
            }
            QToolButton:hover {
                background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                            stop: 0 #3e6aae, stop: 1 #355a9e);
                border-color: #5AA0F2;
            }
            QToolButton:pressed {
            }
            QToolButton:checked {
                background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                            stop: 0 #4e7abe, stop: 1 #456aae);
                border-color: #6AB0FF;
            }
        )");

        // NOTE: Pulse animation removed - Qt QSS doesn't support box-shadow
        // and the effect was non-functional. Keep button styling simple.
    }
    // m_playerWindowToggleAction is already set in menu creation - no need to reassign


    // That's it! Just 3 tools + grid toggle + player window
    // No weather, no effects, no complex controls

    // Initialize fog mode indicator state
    updateFogModeIndicator();

    // Create contextual controls (initially hidden)
    createContextualControls();
}

void MainWindow::createContextualControls()
{
    // SIMPLIFIED: Add spinboxes directly to the main toolbar for space efficiency
    // No separate control strip needed - spinboxes are compact enough

    m_mainToolBar->addSeparator();

    // === Brush Size Spinner (compact control) ===
    QLabel* brushLabel = new QLabel("Brush:", this);
    brushLabel->setStyleSheet("color: #E0E0E0; font-size: 12px; font-weight: 500; padding: 0 4px;");
    m_mainToolBar->addWidget(brushLabel);

    m_fogBrushSizeSpinner = new QSpinBox(this);
    m_fogBrushSizeSpinner->setMinimum(10);
    m_fogBrushSizeSpinner->setMaximum(400);
    m_fogBrushSizeSpinner->setValue(200);  // Match MapDisplay default (200px)
    m_fogBrushSizeSpinner->setSuffix("px");
    m_fogBrushSizeSpinner->setFixedWidth(85);  // Compact width
    m_fogBrushSizeSpinner->setToolTip("<b>Brush Size</b><br>Adjust fog reveal brush diameter<br>Range: 10-400px");
    m_fogBrushSizeSpinner->setStyleSheet(R"(
        QSpinBox {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 4px;
            padding: 4px 8px;
            color: #E0E0E0;
            font-size: 12px;
            font-weight: 500;
        }
        QSpinBox:hover {
            background: rgba(255, 255, 255, 0.08);
            border-color: rgba(74, 144, 226, 0.3);
        }
        QSpinBox:disabled {
            background: rgba(255, 255, 255, 0.02);
            color: #808080;
            border-color: rgba(255, 255, 255, 0.05);
        }
        QSpinBox::up-button, QSpinBox::down-button {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            width: 16px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: rgba(74, 144, 226, 0.2);
        }
        QSpinBox::up-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-bottom: 4px solid #E0E0E0;
            width: 0;
            height: 0;
        }
        QSpinBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 4px solid #E0E0E0;
            width: 0;
            height: 0;
        }
    )");
    m_mainToolBar->addWidget(m_fogBrushSizeSpinner);

    // Use QPointer for safe lambda capture
    QPointer<QSpinBox> safeBrushSpinner = m_fogBrushSizeSpinner;

    // Create debounce timer (50ms delay to prevent rapid-fire updates)
    m_brushSizeDebounceTimer = new QTimer(this);
    m_brushSizeDebounceTimer->setSingleShot(true);
    m_brushSizeDebounceTimer->setInterval(50);

    connect(m_brushSizeDebounceTimer, &QTimer::timeout, [this, safeBrushSpinner]() {
        if (safeBrushSpinner) {
            this->onFogBrushSizeChanged(safeBrushSpinner->value());
        }
    });

    connect(m_fogBrushSizeSpinner, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        // Debounce actual brush size update to prevent rapid-fire
        if (m_brushSizeDebounceTimer) {
            m_brushSizeDebounceTimer->start();
        }
    });

    // === Grid Size Spinner (compact control) ===
    QLabel* gridLabel = new QLabel("Grid:", this);
    gridLabel->setStyleSheet("color: #E0E0E0; font-size: 12px; font-weight: 500; padding: 0 4px;");
    m_mainToolBar->addWidget(gridLabel);

    m_gridSizeSpinner = new QSpinBox(this);
    m_gridSizeSpinner->setMinimum(20);
    m_gridSizeSpinner->setMaximum(500);  // Increased for high-res maps
    m_gridSizeSpinner->setValue(150);  // Default grid size (150px)
    m_gridSizeSpinner->setSuffix("px");
    m_gridSizeSpinner->setSingleStep(10);  // Per CLAUDE.md spec: 10px increments
    m_gridSizeSpinner->setFixedWidth(85);  // Match brush spinner width
    m_gridSizeSpinner->setToolTip("<b>Grid Size</b><br>Adjust grid cell size<br>Range: 20-500px");
    m_gridSizeSpinner->setStyleSheet(R"(
        QSpinBox {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 4px;
            padding: 4px 8px;
            color: #E0E0E0;
            font-size: 12px;
            font-weight: 500;
        }
        QSpinBox:hover {
            background: rgba(255, 255, 255, 0.08);
            border-color: rgba(74, 144, 226, 0.3);
        }
        QSpinBox:disabled {
            background: rgba(255, 255, 255, 0.02);
            color: #808080;
            border-color: rgba(255, 255, 255, 0.05);
        }
        QSpinBox::up-button, QSpinBox::down-button {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            width: 16px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: rgba(74, 144, 226, 0.2);
        }
        QSpinBox::up-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-bottom: 4px solid #E0E0E0;
            width: 0;
            height: 0;
        }
        QSpinBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 4px solid #E0E0E0;
            width: 0;
            height: 0;
        }
    )");
    m_mainToolBar->addWidget(m_gridSizeSpinner);

    // Use QPointer for safe lambda capture
    QPointer<QSpinBox> safeGridSpinner = m_gridSizeSpinner;

    // Create debounce timer (50ms delay to prevent rapid-fire updates)
    m_gridSizeDebounceTimer = new QTimer(this);
    m_gridSizeDebounceTimer->setSingleShot(true);
    m_gridSizeDebounceTimer->setInterval(50);

    connect(m_gridSizeDebounceTimer, &QTimer::timeout, [this, safeGridSpinner]() {
        if (safeGridSpinner) {
            this->onGridSizeChanged(safeGridSpinner->value());
        }
    });

    connect(m_gridSizeSpinner, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        // Debounce actual grid update to prevent rapid-fire
        if (m_gridSizeDebounceTimer) {
            m_gridSizeDebounceTimer->start();
        }
    });

    // GLANCEABLE DESIGN: All controls always visible, disabled when inactive
    // Initially disable spinners until their features are active
    if (m_fogBrushSizeSpinner) {
        m_fogBrushSizeSpinner->setEnabled(false);  // Enable when Fog Mode ON
    }
    if (m_gridSizeSpinner) {
        m_gridSizeSpinner->setEnabled(false);  // Enable when Grid ON
        // Default 150px is already set above - grid overlay will use this value
    }
}

void MainWindow::updateContextualToolbarControls()
{
    if (!m_mainToolBar) return;

    // === GLANCEABLE DESIGN (CLAUDE.md) ===
    // All sliders always visible - user can see what's possible at a glance
    // Disabled state makes it obvious when controls are inactive

    // === Brush Size Spinner ===
    // Always visible, enabled when Fog Mode ON
    if (m_fogBrushSizeSpinner) {
        m_fogBrushSizeSpinner->setEnabled(m_mapDisplay && m_mapDisplay->isFogEnabled());
    }

    // === Grid Size Spinner ===
    // Always visible, enabled when Grid ON
    if (m_gridSizeSpinner) {
        m_gridSizeSpinner->setEnabled(m_mapDisplay && m_mapDisplay->isGridEnabled());
    }

    // === Reset Fog Button ===
    // Always visible, enabled when Fog Mode is ON
    if (m_resetFogAction) {
        m_resetFogAction->setEnabled(m_mapDisplay && m_mapDisplay->isFogEnabled());

        // Apply danger styling (always, since button always visible)
        if (m_mainToolBar) {
            if (auto* btn = qobject_cast<QToolButton*>(m_mainToolBar->widgetForAction(m_resetFogAction))) {
                btn->setStyleSheet(R"(
                    QToolButton {
                        background: rgba(220, 53, 69, 0.15);
                        border: 1px solid rgba(220, 53, 69, 0.3);
                        border-radius: 6px;
                        padding: 8px 16px;
                        color: #ff6b6b;
                        font-size: 13px;
                        font-weight: 500;
                    }
                    QToolButton:hover:enabled {
                        background: rgba(220, 53, 69, 0.25);
                        border-color: rgba(220, 53, 69, 0.5);
                        color: #ff8787;
                    }
                    QToolButton:pressed:enabled {
                        background: rgba(220, 53, 69, 0.35);
                    }
                    QToolButton:disabled {
                        background: rgba(220, 53, 69, 0.05);
                        border-color: rgba(220, 53, 69, 0.1);
                        color: rgba(255, 107, 107, 0.3);
                    }
                )");
            }
        }
    }
}

void MainWindow::createZoomToolbar()
{
    // Zoom toolbar removed - zoom controls available via keyboard shortcuts only
    // Scroll wheel for zoom, 0 for fit to screen, +/- for zoom in/out
}

void MainWindow::setupToolbox()
{
    m_toolboxWidget = new ToolboxWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolboxWidget);
    m_toolboxWidget->hide();  // Hidden by default — toolbar provides primary controls

    m_toolboxWidget->setMapDisplay(m_mapDisplay);
    m_toolboxWidget->setPlayerWindow(m_playerWindow);

    connect(m_toolboxWidget, &ToolboxWidget::loadMapRequested, this, &MainWindow::loadMap);
    connect(m_toolboxWidget, &ToolboxWidget::togglePlayerWindowRequested, this, &MainWindow::togglePlayerWindow);
    connect(m_toolboxWidget, &ToolboxWidget::toggleGridRequested, this, &MainWindow::toggleGrid);
    connect(m_toolboxWidget, &ToolboxWidget::toggleGridTypeRequested, this, &MainWindow::toggleGridType);
    connect(m_toolboxWidget, &ToolboxWidget::toggleFogOfWarRequested, this, &MainWindow::toggleFogOfWar);
    connect(m_toolboxWidget, &ToolboxWidget::resetFogOfWarRequested, this, &MainWindow::resetFogOfWar);
    connect(m_toolboxWidget, &ToolboxWidget::fitToScreenRequested, this, &MainWindow::fitToScreen);
    connect(m_toolboxWidget, &ToolboxWidget::zoomInRequested, this, &MainWindow::zoomIn);
    connect(m_toolboxWidget, &ToolboxWidget::zoomOutRequested, this, &MainWindow::zoomOut);
    connect(m_toolboxWidget, &ToolboxWidget::togglePlayerViewModeRequested, this, &MainWindow::togglePlayerViewMode);
    // Undo/Redo removed from toolbox - using Edit menu only

    connect(m_toolboxWidget, &ToolboxWidget::zoomPresetRequested, [this](qreal value) {
        if (m_mapDisplay) m_mapDisplay->zoomToPreset(value);
    });

    connect(m_toolboxWidget, &ToolboxWidget::fogToolModeChanged, [this](FogToolMode mode) {
        setFogToolMode(mode);
    });

    connect(m_toolboxWidget, &ToolboxWidget::gridSizeChanged, [this](int size) {
        if (m_mapDisplay && m_mapDisplay->getGridOverlay()) {
            m_mapDisplay->getGridOverlay()->setGridSize(size);
        }
    });

    connect(m_toolboxWidget, &ToolboxWidget::fogBrushSizeChanged, [](int size) {
        Q_UNUSED(size);
        // TODO: Implement brush size handling in fog system
    });

    connect(m_toolboxWidget, &ToolboxWidget::gmOpacityChanged, [this](int opacity) {
        if (m_mapDisplay && m_mapDisplay->getFogOverlay()) {
            m_mapDisplay->getFogOverlay()->setGMOpacity(opacity / 100.0);
        }
    });

    connect(m_toolboxWidget, &ToolboxWidget::beaconColorChanged, [this](const QColor& color) {
        if (m_mapDisplay) {
            m_mapDisplay->setBeaconColor(color);
        }
        // Persist the color selection
        SettingsManager::instance().saveGMBeaconColor(color);
    });

    // Load saved beacon color from settings and apply to both toolbox and MapDisplay
    QColor savedBeaconColor = SettingsManager::instance().loadGMBeaconColor();
    m_toolboxWidget->setBeaconColor(savedBeaconColor);
    if (m_mapDisplay) {
        m_mapDisplay->setBeaconColor(savedBeaconColor);
    }

    m_toolboxWidget->updateGridStatus(m_mapDisplay && m_mapDisplay->isGridEnabled());
    m_toolboxWidget->updateFogStatus(m_mapDisplay->isFogEnabled());
    m_toolboxWidget->updatePlayerViewStatus(m_playerViewModeEnabled);
}

void MainWindow::loadMap()
{
    // Load the last used directory from settings, default to home directory
    QString lastDirectory = SettingsManager::instance().loadLastDirectory();

    QString fileName = QFileDialog::getOpenFileName(this,
        "Load Map Image", lastDirectory,
        "Map Files (*.png *.jpg *.jpeg *.webp *.bmp *.dd2vtt *.uvtt *.df2vtt);;Image Files (*.png *.jpg *.jpeg *.webp *.bmp);;VTT Files (*.dd2vtt *.uvtt *.df2vtt);;All Files (*)");

    if (!fileName.isEmpty()) {
        // Save the directory for future use
        QFileInfo fileInfo(fileName);
        QString selectedDirectory = fileInfo.absolutePath();
        SettingsManager::instance().saveLastDirectory(selectedDirectory);

        loadMapFile(fileName);
    }
}

void MainWindow::loadMapFile(const QString& path)
{
    // MapDisplay and ToolManager are now created eagerly in setupMinimalUI
    // Only need to create TabsController if it doesn't exist yet

    // Create TabsController on first map load
    if (!m_tabsController) {

        m_tabsController = new TabsController(this);
        m_tabsController->attach(m_tabBar, m_mapDisplay, MAX_TABS);

        // Connect signals
        connect(m_tabsController, &TabsController::requestShowProgress,
                this, [this](const QString& fileName, qint64 fileSize) {
                    showLoadProgress(fileName, fileSize);
                });
        connect(m_tabsController, &TabsController::requestHideProgress,
                this, &MainWindow::hideLoadProgress);
        connect(m_tabsController, &TabsController::requestStatus,
                this, [this](const QString& message, int timeout) {
                    if (auto* toast = ToastNotification::instance(this)) {
                        toast->showMessage(message, ToastNotification::Type::Info, timeout);
                    }
                });
        connect(m_tabsController, &TabsController::currentMapPathChanged,
                this, [this](const QString& mapPath) {
                    // Update fog autosave controller with new map path and load existing fog state
                    if (m_fogAutosaveController) {
                        m_fogAutosaveController->setCurrentMapPath(mapPath);
                        if (m_mapDisplay && m_mapDisplay->isFogEnabled()) {
                            m_fogAutosaveController->loadFromFile();
                        }
                    }

                    // Reset rotation to default when loading a new map
                    m_mapRotation = 0;
                    m_playerRotation = 0;
                    if (m_mapDisplay) {
                        m_mapDisplay->resetTransform();
                        m_mapDisplay->scale(m_mapDisplay->getZoomLevel(), m_mapDisplay->getZoomLevel());
                    }
                    if (m_playerWindow) {
                        m_playerWindow->setRotation(0);  // Reset player rotation via proper method
                    }
                    updateStatusIndicators();

                    // Update window title with current map name
                    if (!mapPath.isEmpty()) {
                        setWindowTitle(QString("Crit VTT — %1").arg(QFileInfo(mapPath).fileName()));
                    }
                });
        // CRITICAL FIX: Connect requestAddRecent signal to add files to recent files menu
        connect(m_tabsController, &TabsController::requestAddRecent,
                this, &MainWindow::addToRecentFiles, Qt::QueuedConnection);
    }

    if (m_tabsController) {
        m_tabsController->loadMapFile(path);
    }
}

void MainWindow::togglePlayerWindow()
{
    if (!m_playerWindow) {
        m_playerWindow = new PlayerWindow(m_mapDisplay);
        m_playerWindow->setWindowTitle("Crit VTT - Player Display");
        m_playerWindow->resize(1024, 768);

        ensurePlayerWindowConnections();
    }

    ensurePlayerWindowConnections();

    if (m_playerWindow->isVisible()) {
        // Exit fullscreen before hiding to avoid black screen issue
        if (m_playerWindow->isFullScreen()) {
            m_playerWindow->showNormal();
        }
        m_playerWindow->hide();
        // Show toast notification
        if (auto* toast = ToastNotification::instance(this)) {
            toast->showMessage("TV Display closed", ToastNotification::Type::Info, 2000);
        }
    } else {
        m_playerWindow->show();

        m_playerWindow->forceRefresh();

        // Sync rotation when player window opens (if sync enabled)
        if (m_syncRotationToPlayer) {
            m_playerRotation = m_mapRotation;
            m_playerWindow->setRotation(m_playerRotation);
        }

        qreal currentZoom = m_mapDisplay->getZoomLevel();
        QPointF centerPoint = m_mapDisplay->mapToScene(m_mapDisplay->rect().center());
        m_playerWindow->syncZoom(currentZoom, centerPoint);

        // Auto-send to secondary display if available
        QScreen* secondaryScreen = PlayerWindow::findSecondaryScreen();
        if (secondaryScreen) {
            m_playerWindow->moveToSecondaryDisplay();
            if (auto* toast = ToastNotification::instance(this)) {
                toast->showMessage("TV Display sent to secondary monitor (fullscreen)",
                                  ToastNotification::Type::Success, 3000);
            }
        } else {
            if (auto* toast = ToastNotification::instance(this)) {
                toast->showMessage("TV Display opened - connect TV for auto-fullscreen",
                                  ToastNotification::Type::Success, 3000);
            }
        }

    }

    // Update the toggle button state
    if (m_playerWindowToggleAction) {
        m_playerWindowToggleAction->setChecked(m_playerWindow && m_playerWindow->isVisible());
    }
}

void MainWindow::updateSendToDisplayMenu()
{
    if (!m_sendToDisplayMenu) return;

    // Clear existing actions
    m_sendToDisplayMenu->clear();

    QList<QScreen*> screens = QGuiApplication::screens();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    if (screens.size() <= 1) {
        // Only one monitor - show informative disabled action
        QAction* noDisplayAction = m_sendToDisplayMenu->addAction("No secondary display detected");
        noDisplayAction->setEnabled(false);
        return;
    }

    // Add action for each available screen
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        QString displayName;

        // Create user-friendly name
        if (screen == primaryScreen) {
            displayName = QString("Display %1 (Primary) - %2x%3")
                .arg(i + 1)
                .arg(screen->geometry().width())
                .arg(screen->geometry().height());
        } else {
            displayName = QString("Display %1 - %2x%3")
                .arg(i + 1)
                .arg(screen->geometry().width())
                .arg(screen->geometry().height());
        }

        QAction* action = m_sendToDisplayMenu->addAction(displayName);

        // Capture screen pointer for lambda
        connect(action, &QAction::triggered, this, [this, screen]() {
            sendPlayerWindowToScreen(screen);
        });
    }

    // Add separator and enable state info
    m_sendToDisplayMenu->addSeparator();
    if (!m_playerWindow || !m_playerWindow->isVisible()) {
        QAction* infoAction = m_sendToDisplayMenu->addAction("Open Player Window first (P)");
        infoAction->setEnabled(false);
    }
}

void MainWindow::sendPlayerWindowToScreen(QScreen* screen)
{
    if (!screen) return;

    // Create player window if it doesn't exist
    if (!m_playerWindow) {
        m_playerWindow = new PlayerWindow(m_mapDisplay);
        m_playerWindow->setWindowTitle("Crit VTT - Player Display");
        ensurePlayerWindowConnections();
    }

    // Show the window first if not visible
    if (!m_playerWindow->isVisible()) {
        m_playerWindow->show();
        m_playerWindow->forceRefresh();
    }

    // Move to specified screen and go fullscreen
    m_playerWindow->moveToScreen(screen, true);

    // Show toast notification
    if (auto* toast = ToastNotification::instance(this)) {
        QString screenName = (screen == QGuiApplication::primaryScreen())
            ? "primary display"
            : QString("display %1").arg(QGuiApplication::screens().indexOf(screen) + 1);
        toast->showMessage(QString("Player window sent to %1").arg(screenName),
                          ToastNotification::Type::Success, 3000);
    }

    // Update toggle state
    if (m_playerWindowToggleAction) {
        m_playerWindowToggleAction->setChecked(true);
    }
}

void MainWindow::syncViewToPlayers()
{
    if (!m_playerWindow || !m_playerWindow->isVisible()) {
        // Open player window first
        if (auto* toast = ToastNotification::instance(this)) {
            toast->showMessage("Open Player Window first (P)", ToastNotification::Type::Warning, 3000);
        }
        return;
    }

    if (!m_mapDisplay) return;

    // Get DM's current view
    qreal currentZoom = m_mapDisplay->getZoomLevel();
    QPointF centerPoint = m_mapDisplay->mapToScene(m_mapDisplay->rect().center());

    // Push to player window (overrides auto-fit)
    m_playerWindow->syncViewFromDM(currentZoom, centerPoint);

    // Show toast notification
    if (auto* toast = ToastNotification::instance(this)) {
        toast->showMessage("View synced to players", ToastNotification::Type::Success, 2000);
    }
}

void MainWindow::resetPlayerAutoFit()
{
    if (!m_playerWindow || !m_playerWindow->isVisible()) {
        if (auto* toast = ToastNotification::instance(this)) {
            toast->showMessage("Player Window not open", ToastNotification::Type::Warning, 2000);
        }
        return;
    }

    // Reset to auto-fit mode
    m_playerWindow->resetToAutoFit();

    // Show toast notification
    if (auto* toast = ToastNotification::instance(this)) {
        toast->showMessage("Player view reset to auto-fit", ToastNotification::Type::Success, 2000);
    }
}

void MainWindow::toggleGrid()
{
    // MEMORY OPTIMIZATION: Create grid controller on first use
    if (!m_gridController && m_mapDisplay) {
        m_gridController = new GridController(this);
        m_gridController->attachToMainWindow(this);
        m_gridController->setMapDisplay(m_mapDisplay);
    }

    if (m_gridController) {
        m_gridController->toggleGrid();

        // Show toast notification with correct NEW state
        if (auto* toast = ToastNotification::instance(this)) {
            const bool gridOn = m_gridController->isGridEnabled();
            QString message = gridOn ? "Grid shown" : "Grid hidden";
            toast->showMessage(message, ToastNotification::Type::Info, 2000);
        }
        updateStatusIndicators();
        if (m_playerWindowController) {
            m_playerWindowController->updatePlayerWindow();
        }

        // Update contextual toolbar controls
        updateContextualToolbarControls();
    }
}


void MainWindow::openPreferences()
{
    SettingsDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Settings are automatically saved and applied by the dialog
        // Additional application-specific updates can be performed here

        // Update any UI elements that depend on the new settings
        // For example, refresh grid display, fog settings, etc.
        if (m_mapDisplay) {
            m_mapDisplay->update();
            // Re-apply wheel zoom preference
            m_mapDisplay->setZoomControlsEnabled(SettingsManager::instance().loadWheelZoomEnabled());

            // Sync with player window
            if (m_playerWindow) {
                m_playerWindow->forceRefresh();
            }
        }

        showAutosaveNotification("Preferences updated successfully");
    }
}

void MainWindow::onGridSizeChanged(int value)
{
    if (!m_mapDisplay || !m_mapDisplay->getGridOverlay()) {
        return;
    }

    // Update grid size on main display
    m_mapDisplay->getGridOverlay()->setGridSize(value);

    // Sync with player window if open
    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }

    // Show status message
    statusBar()->showMessage(QString("Grid size: %1 pixels").arg(value), 2000);
}

void MainWindow::onFogBrushSizeChanged(int value)
{
    if (!m_mapDisplay) {
        return;
    }

    // Update brush size on main display
    m_mapDisplay->setFogBrushSize(value);

    if (m_fogToolsController) {
        m_fogToolsController->setBrushSize(value);
    }

    // Sync with player window if open
    if (m_playerWindow && m_playerWindow->getMapDisplay()) {
        m_playerWindow->getMapDisplay()->setFogBrushSize(value);
    }

    // Show status message
    statusBar()->showMessage(QString("Fog brush size: %1 pixels").arg(value), 2000);

    // Update tool overlay chip with new brush size
    if (auto* toolOverlay = m_mapDisplay->findChild<ToolOverlayWidget*>("toolOverlay")) {
        QString modeText = getFogToolModeText(m_fogToolMode);
        QString hints = "(R/H - Shift+R/H)";
        toolOverlay->setText(QString("Tool: %1 %2 — %3px").arg(modeText, hints).arg(value));
    }
}

void MainWindow::setSmallBrush()
{
    const int smallSize = 25;
    if (m_fogBrushSizeSpinner) {
        m_fogBrushSizeSpinner->setValue(smallSize);
    }
    onFogBrushSizeChanged(smallSize);
}

void MainWindow::setMediumBrush()
{
    const int mediumSize = 50;
    if (m_fogBrushSizeSpinner) {
        m_fogBrushSizeSpinner->setValue(mediumSize);
    }
    onFogBrushSizeChanged(mediumSize);
}

void MainWindow::setLargeBrush()
{
    const int largeSize = 100;
    if (m_fogBrushSizeSpinner) {
        m_fogBrushSizeSpinner->setValue(largeSize);
    }
    onFogBrushSizeChanged(largeSize);
}

void MainWindow::onGMOpacityChanged(int value)
{
    if (!m_mapDisplay) {
        return;
    }

    // Convert percentage to opacity value (0-100 -> 0.0-1.0)
    qreal opacity = value / 100.0;

    // Update fog opacity on main display (DM view)
    m_mapDisplay->getFogOverlay()->setGMOpacity(opacity);

    // Player window should always use 100% opacity (will be handled in PlayerWindow setup)
}

void MainWindow::updateGridSizeSlider()
{
    if (!m_gridSizeSpinner || !m_mapDisplay || !m_mapDisplay->getGridOverlay()) {
        return;
    }

    int currentGridSize = m_mapDisplay->getGridOverlay()->getGridSize();

    // Block signals to prevent recursive calls
    m_gridSizeSpinner->blockSignals(true);
    m_gridSizeSpinner->setValue(currentGridSize);
    m_gridSizeSpinner->blockSignals(false);
}

void MainWindow::toggleFogOfWar()
{
    // Toggle fog via MapDisplay — the single source of truth for fog state
    const bool newFogState = !m_mapDisplay->isFogEnabled();
    m_mapDisplay->setFogEnabled(newFogState);

    // Enable/disable fog tool buttons based on Fog Mode state (respecting lock)
    bool fogControlsEnabled = newFogState && !m_fogLocked;
    if (m_fogBrushAction) {
        m_fogBrushAction->setEnabled(fogControlsEnabled);
    }
    if (m_fogRectAction) {
        m_fogRectAction->setEnabled(fogControlsEnabled);
    }
    if (m_resetFogAction) {
        m_resetFogAction->setEnabled(fogControlsEnabled);
    }
    if (m_fogHideToggleAction) {
        m_fogHideToggleAction->setEnabled(newFogState);
    }

    // Auto-select Reveal Brush when fog is enabled
    if (newFogState && m_fogBrushAction) {
        m_fogBrushAction->setChecked(true);
        if (m_toolManager) {
            m_toolManager->setActiveTool(ToolType::FogBrush);
        }
        statusBar()->showMessage("Fog Mode ON - Reveal Brush active - Click/drag to reveal areas", 3000);
    } else {
        statusBar()->showMessage("Fog Mode OFF - Map fully visible to players", 2000);
    }

    // Update contextual toolbar controls (show/hide sliders)
    updateContextualToolbarControls();

    // Update the action's checked state
    if (auto* action = getOrCreateAction("fog_toggle")) {
        action->setChecked(newFogState);
    }

    // Initialize fog autosave controller when fog is first enabled
    if (newFogState && !m_fogAutosaveController) {
        m_fogAutosaveController = new FogAutosaveController(this);
        m_fogAutosaveController->setMapDisplay(m_mapDisplay);

        // Set current map path if we have one
        if (m_tabsController) {
            if (auto* session = m_tabsController->getCurrentSession()) {
                m_fogAutosaveController->setCurrentMapPath(session->filePath());
            }
        }

        // Connect fog change notifications to trigger autosave
        // Disabled: Autosave notifications as requested by user
        // connect(m_fogAutosaveController, &FogAutosaveController::notify,
        //         this, [this](const QString& message) {
        //             if (auto* toast = ToastNotification::instance(this)) {
        //                 toast->showMessage(message, ToastNotification::Type::Info, 1000);
        //             }
        //         });
    }

    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }

    // Save fog state to settings
    SettingsManager::instance().saveFogEnabled(newFogState);

    // Show toast notification
    if (auto* toast = ToastNotification::instance(this)) {
        QString message = newFogState ? "Fog of War enabled" : "Fog of War disabled";
        ToastNotification::Type type = newFogState ? ToastNotification::Type::Warning : ToastNotification::Type::Success;
        toast->showMessage(message, type, 2000);
    }

    // Update status indicators
    updateStatusIndicators();

    statusBar()->showMessage(newFogState ? "Fog of War enabled" : "Fog of War disabled", 2000);
}

void MainWindow::clearFogOfWar()
{
    m_mapDisplay->clearFog();

    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }

    // Show toast notification
    if (auto* toast = ToastNotification::instance(this)) {
        toast->showMessage("Fog of War cleared", ToastNotification::Type::Success, 2000);
    }
}


void MainWindow::resetFogOfWar()
{
    // Show confirmation dialog to prevent accidental resets
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Reset Fog of War",
        "Reset all fog? This will hide the entire map.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No  // Default to No for safety
    );

    if (reply == QMessageBox::Yes) {
        m_mapDisplay->resetFog();

        if (m_playerWindow) {
            m_playerWindow->forceRefresh();
        }

        statusBar()->showMessage("Fog of War reset - Entire map hidden from players", 2000);
    }
}

// Auto-open player window on startup
void MainWindow::autoOpenPlayerWindow()
{
    try {
        // Only auto-open if we haven't created the window yet
        if (!m_playerWindow) {
            // Validate map display is ready
            if (!m_mapDisplay) {
                DebugConsole::warning("MapDisplay not ready, skipping player window auto-open", "UI");
                return;
            }

            // Create player window with error handling
            m_playerWindow = new PlayerWindow(m_mapDisplay);
            if (!m_playerWindow) {
                DebugConsole::warning("Failed to create player window", "UI");
                return;
            }

            m_playerWindow->setWindowTitle("Crit VTT - Player Display");

            ensurePlayerWindowConnections();

            // Position the window with validation
            positionPlayerWindow();

            // Show window with error handling
            m_playerWindow->show();

            // Ensure the initial player view matches the DM display immediately
            m_playerWindow->forceRefresh();
            if (m_mapDisplay) {
                qreal currentZoom = m_mapDisplay->getZoomLevel();
                QPointF centerPoint = m_mapDisplay->mapToScene(m_mapDisplay->rect().center());
                m_playerWindow->syncZoom(currentZoom, centerPoint);
            }

            // Verify window is visible
            if (m_playerWindow->isVisible()) {
                statusBar()->showMessage("Player window opened on secondary display", 3000);
            } else {
                DebugConsole::warning("Player window created but not visible", "UI");
            }
        }
    } catch (const std::exception& e) {
        DebugConsole::warning(QString("Exception during player window auto-open: %1").arg(e.what()), "UI");
        // Clean up on failure
        if (m_playerWindow) {
            delete m_playerWindow;
            m_playerWindow = nullptr;
        }
    }
}

// Position player window on secondary monitor if available
void MainWindow::positionPlayerWindow()
{
    if (!m_playerWindow) return;

    QList<QScreen*> screens = QApplication::screens();

    if (screens.size() > 1) {
        // Place on secondary monitor
        QScreen* secondaryScreen = screens[1];
        QRect screenGeometry = secondaryScreen->geometry();

        // Center the window on the secondary screen
        int width = qMin(1024, screenGeometry.width() - 100);
        int height = qMin(768, screenGeometry.height() - 100);
        int x = screenGeometry.x() + (screenGeometry.width() - width) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - height) / 2;

        m_playerWindow->setGeometry(x, y, width, height);
    } else {
        // Position offset from main window on single monitor
        QPoint mainPos = this->pos();
        m_playerWindow->move(mainPos.x() + 50, mainPos.y() + 50);
        m_playerWindow->resize(1024, 768);
    }
}

void MainWindow::ensurePlayerWindowConnections()
{
    if (!m_mapDisplay || !m_playerWindow) {
        return;
    }

    QObject::connect(
        m_mapDisplay,
        &MapDisplay::fogChanged,
        this,
        &MainWindow::handleDisplayFogChanged,
        Qt::UniqueConnection
    );

    // SYNC FIX: Scene populated (map loaded)
    QObject::connect(
        m_mapDisplay,
        &MapDisplay::scenePopulated,
        this,
        [this]() {
            if (m_playerWindow && m_playerWindow->isVisible()) {
                m_playerWindow->forceRefresh();
            }
            // First-map onboarding toast (once per launch)
            static bool shownTip = false;
            if (!shownTip) {
                shownTip = true;
                if (auto* toast = ToastNotification::instance(this)) {
                    toast->showMessage("Tip: Press F for Fog of War, P to open Player View on TV",
                                       ToastNotification::Type::Info, 5000);
                }
            }
        },
        Qt::UniqueConnection
    );
}

// Show progress dialog for large images
void MainWindow::showLoadProgress(const QString& fileName, qint64 fileSize)
{
    if (!m_loadingOverlay) {
        m_loadingOverlay = new LoadingOverlay(this);
        connect(m_loadingOverlay, &LoadingOverlay::cancelled, this, [this]() {
            // Handle cancellation if needed
            hideLoadProgress();
        });
    }

    // Determine if we can show determinate progress
    bool isIndeterminate = (fileSize <= 0);
    QString message;
    
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        if (fileSize > 0) {
            QString sizeStr = QString::number(fileSize / (1024.0 * 1024.0), 'f', 1) + " MB";
            message = QString("Loading %1 (%2)...").arg(fileInfo.fileName()).arg(sizeStr);
        } else {
            message = QString("Loading %1...").arg(fileInfo.fileName());
        }
    } else {
        message = "Loading map...";
    }
    
    m_loadingOverlay->showLoading(message, isIndeterminate, 100);
    m_loadTimer.start();
}

void MainWindow::hideLoadProgress()
{
    if (m_loadingOverlay && m_loadingOverlay->isLoading()) {
        m_loadingOverlay->hideLoading();
    }
}

void MainWindow::handleLoadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (m_loadingOverlay && m_loadingOverlay->isLoading() && totalBytes > 0) {
        int percentage = static_cast<int>((bytesRead * 100) / totalBytes);
        QString subMessage = QString("%1 of %2 MB")
            .arg(QString::number(bytesRead / 1024.0 / 1024.0, 'f', 1))
            .arg(QString::number(totalBytes / 1024.0 / 1024.0, 'f', 1));
        m_loadingOverlay->updateProgress(percentage, subMessage);
    }
}

// Drag and drop event handlers
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();
            QFileInfo fileInfo(path);

            // Check if it's an image or supported VTT file
            QStringList imageExtensions = {"png", "jpg", "jpeg", "webp", "bmp"};
            QStringList vttExtensions = {"dd2vtt", "uvtt", "df2vtt"};
            const QString ext = fileInfo.suffix().toLower();
            if (imageExtensions.contains(ext) || vttExtensions.contains(ext)) {
                event->acceptProposedAction();
                m_isDragging = true;
                animateDropFeedback(true);
                statusBar()->showMessage("Drop file to load as map", 2000);
                return;
            }
        }
    }
    event->ignore();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (m_isDragging) {
        event->acceptProposedAction();

        // Update overlay position to follow cursor
        if (m_dropOverlay && m_dropOverlay->isVisible()) {
            QRect rect = centralWidget()->rect();
            rect.adjust(20, 20, -20, -20);
            m_dropOverlay->setGeometry(rect);
        }
    } else {
        event->ignore();
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    if (m_isDragging) {
        m_isDragging = false;
        animateDropFeedback(false);
        statusBar()->clearMessage();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();

            // Animate drop feedback out
            animateDropFeedback(false);
            m_isDragging = false;

            // Load the dropped file with a slight delay for visual effect
            QPointer<MainWindow> dropSelf = this;
            QTimer::singleShot(150, this, [dropSelf, path]() {
                if (dropSelf) dropSelf->loadMapFile(path);
            });

            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void MainWindow::animateDropFeedback(bool entering)
{
    if (!m_dropOverlay) return;

    if (entering) {
        // Position and show overlay without animations
        QRect rect = centralWidget()->rect();
        rect.adjust(20, 20, -20, -20);
        m_dropOverlay->setGeometry(rect);
        m_dropOverlay->show();
        m_dropOverlay->raise();

        // Premium drop feedback with subtle animation feel
        m_dropOverlay->setStyleSheet(
            "QWidget {"
            "    background: qradialgradient("
            "        cx: 0.5, cy: 0.5, radius: 0.8,"
            "        stop: 0 rgba(74, 158, 255, 0.15),"
            "        stop: 0.5 rgba(74, 158, 255, 0.08),"
            "        stop: 1 transparent"
            "    );"
            "    border: 2px dashed #4a9eff;"
            "    border-radius: 12px;"
            "}");
    } else {
        // Simple hide without animation
        m_dropOverlay->hide();

        // Clean up any orphaned animations if they exist
        for (QObject* child : m_dropOverlay->children()) {
            if (QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(child)) {
                anim->stop();
                anim->deleteLater();
            }
        }
    }
}

// Delegated to RecentFilesController
void MainWindow::openRecentFile() {}

void MainWindow::clearRecentFiles() {}

void MainWindow::updateRecentFilesMenu() { if (m_recentFilesController) m_recentFilesController->updateMenu(); }

void MainWindow::addToRecentFiles(const QString& filePath) { if (m_recentFilesController) m_recentFilesController->addToRecent(filePath); }

void MainWindow::setupStatusBar()
{
    // Ultra-minimal status bar with only essential info
    statusBar()->setStyleSheet(R"(
        QStatusBar {
            background: #1a1a1a;
            border-top: 1px solid #2a2a2a;
            padding: 4px;
            font-size: 12px;
            color: #888;
        }
        QStatusBar::item {
            border: none;
        }
    )");

    statusBar()->showMessage("Ready");

    // Create and add ToolStatusWidget as the first permanent widget
    m_toolStatusWidget = new ToolStatusWidget(this);
    statusBar()->addPermanentWidget(m_toolStatusWidget);

    // Minimal status container with only 3 indicators
    m_statusContainer = new QWidget(this);
    m_statusContainer->setStyleSheet(R"(
        QWidget { background: transparent; }
        QLabel {
            color: #666;
            font-size: 11px;
            font-weight: 500;
            padding: 2px 8px;
            margin: 0 2px;
            border-radius: 3px;
        }
        QLabel[active="true"] {
            color: #4A90E2;
            background: rgba(74, 144, 226, 0.1);
        }
    )");

    QHBoxLayout* statusLayout = new QHBoxLayout(m_statusContainer);
    statusLayout->setContentsMargins(0, 0, 8, 0);
    statusLayout->setSpacing(8);

    // Only 3 status indicators: Current Tool | Grid Status | Zoom Level
    m_gridStatusLabel = new QLabel("Grid: Off", m_statusContainer);
    m_gridStatusLabel->setToolTip("Grid overlay (G to toggle)");
    statusLayout->addWidget(m_gridStatusLabel);

    m_zoomStatusLabel = new QLabel("100%", m_statusContainer);
    m_zoomStatusLabel->setToolTip("Zoom level (scroll to zoom)");
    statusLayout->addWidget(m_zoomStatusLabel);

    // Rotation status indicator (shows DM/Player rotation state)
    m_rotationStatusLabel = new QLabel("Rot: 0°", m_statusContainer);
    m_rotationStatusLabel->setToolTip("DM rotation (click rotate button to change)");
    statusLayout->addWidget(m_rotationStatusLabel);

    // Hidden privacy indicator (only shows when active)
    m_privacyStatusLabel = new QLabel("PRIVACY", m_statusContainer);
    m_privacyStatusLabel->setToolTip("Privacy mode active - Player screen protected");
    m_privacyStatusLabel->setStyleSheet(R"(
        QLabel {
            color: #E74C3C;
            background: rgba(231, 76, 60, 0.15);
            font-weight: 600;
            padding: 2px 10px;
            border-radius: 3px;
        }
    )");
    m_privacyStatusLabel->setStyleSheet(
        "QLabel {"
        "   color: #E74C3C;"
        "   font-weight: bold;"
        "   padding: 2px 6px;"
        "   background: rgba(231, 76, 60, 0.1);"
        "   border: 1px solid #E74C3C;"
        "   border-radius: 3px;"
        "}"
    );
    m_privacyStatusLabel->hide(); // Hidden by default
    statusLayout->addWidget(m_privacyStatusLabel);

    // Player sync badge (hidden until Player Window visible)
    m_playerSyncBadge = new QLabel("Synced", m_statusContainer);
    m_playerSyncBadge->setStyleSheet("QLabel { background: #2E7D32; color: white; padding: 2px 6px; border-radius: 6px; }");
    m_playerSyncBadge->setVisible(false);
    statusLayout->addWidget(m_playerSyncBadge);

    statusLayout->addStretch();
    statusBar()->addPermanentWidget(m_statusContainer);

    updateStatusIndicators();
}

void MainWindow::updateStatusIndicators()
{
    if (!m_gridStatusLabel || !m_fogStatusLabel || !m_playerViewStatusLabel || !m_zoomStatusLabel) {
        return;
    }

    // Minimalist grid indicator with subtle state
    const bool gridEnabled = m_mapDisplay && m_mapDisplay->isGridEnabled();
    m_gridStatusLabel->setProperty("active", gridEnabled);
    if (gridEnabled && m_mapDisplay->getGridOverlay()) {
        GridOverlay* grid = m_mapDisplay->getGridOverlay();
        m_gridStatusLabel->setToolTip(QString("Grid: %1px = %2ft")
            .arg(grid->getGridSize())
            .arg(grid->getFeetPerSquare(), 0, 'f', 1));
    } else {
        m_gridStatusLabel->setToolTip("Grid overlay (Ctrl+G)");
    }

    // Minimalist fog indicator
    const bool fogOn = m_mapDisplay && m_mapDisplay->isFogEnabled();
    m_fogStatusLabel->setProperty("active", fogOn);
    m_fogStatusLabel->setToolTip(fogOn ? "Fog enabled" : "Fog disabled (Ctrl+F)");

    // Player view mode indicator
    m_playerViewStatusLabel->setProperty("active", m_playerViewModeEnabled);
    m_playerViewStatusLabel->setToolTip(m_playerViewModeEnabled ? "Player view mode active" : "Player view mode (Ctrl+P)");

    // Clean zoom display
    qreal zoomLevel = 100.0;
    if (m_mapDisplay) {
        zoomLevel = m_mapDisplay->transform().m11() * 100.0;
    }
    m_zoomStatusLabel->setText(QString("%1%").arg(qRound(zoomLevel)));
    m_zoomStatusLabel->setProperty("active", zoomLevel != 100.0);

    // Rotation status - show DM and Player rotation if different
    if (m_rotationStatusLabel) {
        if (m_syncRotationToPlayer) {
            // Synced: just show DM rotation
            m_rotationStatusLabel->setText(QString("Rot: %1°").arg(m_mapRotation));
            m_rotationStatusLabel->setToolTip("Rotation (synced to player)");
            m_rotationStatusLabel->setStyleSheet("");  // Default style
        } else {
            // Independent: show both if different
            if (m_mapRotation != m_playerRotation) {
                m_rotationStatusLabel->setText(QString("DM:%1° / P:%2°").arg(m_mapRotation).arg(m_playerRotation));
                m_rotationStatusLabel->setToolTip("DM and Player rotations are independent");
                m_rotationStatusLabel->setStyleSheet("QLabel { color: #FFA500; }");  // Orange to indicate difference
            } else {
                m_rotationStatusLabel->setText(QString("Rot: %1°").arg(m_mapRotation));
                m_rotationStatusLabel->setToolTip("Rotation (sync disabled, but currently matching)");
                m_rotationStatusLabel->setStyleSheet("");
            }
        }
        m_rotationStatusLabel->setProperty("active", m_mapRotation != 0 || m_playerRotation != 0);
    }

    // Refresh styles with smooth transition
    m_gridStatusLabel->style()->unpolish(m_gridStatusLabel);
    m_gridStatusLabel->style()->polish(m_gridStatusLabel);
    m_fogStatusLabel->style()->unpolish(m_fogStatusLabel);
    m_fogStatusLabel->style()->polish(m_fogStatusLabel);
    m_playerViewStatusLabel->style()->unpolish(m_playerViewStatusLabel);
    m_playerViewStatusLabel->style()->polish(m_playerViewStatusLabel);
    m_zoomStatusLabel->style()->unpolish(m_zoomStatusLabel);
    m_zoomStatusLabel->style()->polish(m_zoomStatusLabel);
}

void MainWindow::updateZoomStatus()
{
    if (m_mapDisplay) {
        qreal zoomLevel = m_mapDisplay->transform().m11() * 100.0;
        int percentage = qRound(zoomLevel);

        if (m_zoomStatusLabel) {
            m_zoomStatusLabel->setText(QString("Zoom: %1%").arg(percentage));
        }

        if (m_zoomPercentageLabel) {
            m_zoomPercentageLabel->setText(QString("%1%").arg(percentage));
        }
        if (m_toolboxWidget) {
            m_toolboxWidget->updateZoomStatus(QString("%1%").arg(percentage));
        }
    }
}

void MainWindow::syncZoomWithPlayer(qreal zoomLevel)
{
    if (m_playerWindow && m_playerWindow->isVisible()) {
        // Get the center point of the main window's view for consistent centering
        QPointF centerPoint = m_mapDisplay->mapToScene(m_mapDisplay->rect().center());
        m_playerWindow->syncZoom(zoomLevel, centerPoint);
        if (m_playerSyncBadge) m_playerSyncBadge->setVisible(true);
    }
}

QString MainWindow::getFogFilePath(const QString& mapPath) const
{
    if (mapPath.isEmpty()) {
        return QString();
    }
    
    // Generate fog file path by appending .fog to the map file
    return mapPath + ".fog";
}

void MainWindow::loadFogState(const QString& mapPath)
{
    if (mapPath.isEmpty() || !m_mapDisplay) {
        return;
    }
    
    QString fogFilePath = getFogFilePath(mapPath);
    QFileInfo fogFileInfo(fogFilePath);
    
    if (!fogFileInfo.exists() || !fogFileInfo.isReadable()) {
        // No fog file exists, start with clean fog
        return;
    }
    
    // Read fog data from file
    QFile fogFile(fogFilePath);
    if (!fogFile.open(QIODevice::ReadOnly)) {
        DebugConsole::warning(QString("Failed to open fog file for reading: %1").arg(fogFilePath), "UI");
        return;
    }
    
    QByteArray fogData = fogFile.readAll();
    fogFile.close();
    
    // Load fog state into the fog overlay through MapDisplay
    if (m_mapDisplay->loadFogState(fogData)) {
        showAutosaveNotification(QString("Loaded fog state from %1").arg(QFileInfo(fogFilePath).fileName()));
    } else {
        DebugConsole::warning(QString("Failed to load fog state from: %1").arg(fogFilePath), "UI");
    }
}

void MainWindow::saveFogState()
{
    if (m_fogAutosaveController) {
        m_fogAutosaveController->saveNow();
    }
}

void MainWindow::scheduleFogSave()
{
    if (m_fogAutosaveController) {
        m_fogAutosaveController->onFogChanged();
    }
}

void MainWindow::onFogChanged()
{
    scheduleFogSave();
    updateUndoRedoButtons();
}

void MainWindow::handleDisplayFogChanged()
{
    onFogChanged();

    if (m_playerWindow && m_playerWindow->isVisible()) {
        m_playerWindow->forceRefresh();
    }
}

void MainWindow::quickSaveFogState()
{
    if (!m_mapDisplay) {
        return;
    }

    // Get the current map path
    QString mapPath = m_currentMapPath;
    if (mapPath.isEmpty()) {
        statusBar()->showMessage("No map loaded - cannot save fog state", 2000);
        return;
    }

    // Create quick save path (use .quickfog extension)
    QString quickSavePath = mapPath + ".quickfog";

    // Save the current fog state
    QByteArray fogData = m_mapDisplay->saveFogState();

    QFile file(quickSavePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(fogData);
        file.close();
        showAutosaveNotification(QString("Quick saved fog state to %1").arg(QFileInfo(quickSavePath).fileName()));
    } else {
        statusBar()->showMessage("Failed to save fog state", 2000);
    }
}

void MainWindow::quickRestoreFogState()
{
    if (!m_mapDisplay) {
        return;
    }

    // Get the current map path
    QString mapPath = m_currentMapPath;
    if (mapPath.isEmpty()) {
        statusBar()->showMessage("No map loaded - cannot restore fog state", 2000);
        return;
    }

    // Create quick save path (use .quickfog extension)
    QString quickSavePath = mapPath + ".quickfog";

    // Check if quick save exists
    if (!QFile::exists(quickSavePath)) {
        statusBar()->showMessage("No quick save found for this map", 2000);
        return;
    }

    // Load the saved fog state
    QFile file(quickSavePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fogData = file.readAll();
        file.close();

        if (m_mapDisplay->loadFogState(fogData)) {
            showAutosaveNotification(QString("Restored fog state from %1").arg(QFileInfo(quickSavePath).fileName()));
        } else {
            statusBar()->showMessage("Failed to restore fog state", 2000);
        }
    } else {
        statusBar()->showMessage("Failed to read fog state file", 2000);
    }
}

void MainWindow::showAutosaveNotification(const QString& message)
{
    if (!message.isEmpty()) {
        statusBar()->showMessage(message, 2000); // Show for 2 seconds
    }
}

void MainWindow::fitToScreen()
{
    if (m_mapDisplay) {
        // Use the MapDisplay's fitMapToView method which properly handles aspect ratio
        m_mapDisplay->fitMapToView();
        updateZoomStatus();
        statusBar()->showMessage("Fit to screen", 2000);

        // Update zoom spinner
        if (m_zoomSpinner) {
            qreal currentScale = m_mapDisplay->transform().m11();
            int newValue = qRound(currentScale * 100);
            m_updatingZoomSpinner = true;
            m_zoomSpinner->setValue(newValue);
            m_updatingZoomSpinner = false;
        }

        // Fit player window to its own viewport
        if (m_playerWindow && m_playerWindow->isVisible()) {
            m_playerWindow->fitToView();
        }
    }
}

void MainWindow::centerOnMap()
{
    if (m_mapDisplay && m_mapDisplay->scene()) {
        // Get the map bounds and center on it
        QRectF sceneRect = m_mapDisplay->scene()->sceneRect();
        if (!sceneRect.isEmpty()) {
            m_mapDisplay->centerOn(sceneRect.center());
            statusBar()->showMessage("View centered on map", 2000);
        }
    }
}

void MainWindow::toggleFogLock()
{
    m_fogLocked = !m_fogLocked;

    // Update lock action icon
    if (m_lockFogAction) {
        m_lockFogAction->setIcon(QIcon(m_fogLocked ? ":/icons/lock.svg" : ":/icons/unlock.svg"));
        m_lockFogAction->setToolTip(m_fogLocked ?
            "<b>Fog Locked</b><br>Click to unlock fog editing" :
            "<b>Fog Unlocked</b><br>Click to lock fog editing");
        m_lockFogAction->setChecked(m_fogLocked);
    }

    // Disable/enable fog controls based on lock state
    bool fogControlsEnabled = (m_mapDisplay && m_mapDisplay->isFogEnabled()) && !m_fogLocked;
    if (m_fogBrushAction) m_fogBrushAction->setEnabled(fogControlsEnabled);
    if (m_fogRectAction) m_fogRectAction->setEnabled(fogControlsEnabled);
    if (m_resetFogAction) m_resetFogAction->setEnabled(fogControlsEnabled);
    if (m_fogBrushSizeSpinner) m_fogBrushSizeSpinner->setEnabled(fogControlsEnabled);

    statusBar()->showMessage(m_fogLocked ? "Fog editing locked" : "Fog editing unlocked", 2000);
}

void MainWindow::toggleGridLock()
{
    m_gridLocked = !m_gridLocked;

    // Update lock action icon
    if (m_lockGridAction) {
        m_lockGridAction->setIcon(QIcon(m_gridLocked ? ":/icons/lock.svg" : ":/icons/unlock.svg"));
        m_lockGridAction->setToolTip(m_gridLocked ?
            "<b>Grid Locked</b><br>Click to unlock grid editing" :
            "<b>Grid Unlocked</b><br>Click to lock grid editing");
        m_lockGridAction->setChecked(m_gridLocked);
    }

    // Disable/enable grid controls based on lock state
    if (m_gridSizeSpinner) m_gridSizeSpinner->setEnabled((m_mapDisplay && m_mapDisplay->isGridEnabled()) && !m_gridLocked);
    if (m_toggleGridAction) m_toggleGridAction->setEnabled(!m_gridLocked);

    statusBar()->showMessage(m_gridLocked ? "Grid editing locked" : "Grid editing unlocked", 2000);
}

void MainWindow::rotateMap()
{
    // Rotate 90° clockwise
    m_mapRotation = (m_mapRotation + 90) % 360;

    if (m_mapDisplay) {
        // Reset and apply zoom + rotation together
        qreal zoom = m_mapDisplay->getZoomLevel();
        m_mapDisplay->resetTransform();
        m_mapDisplay->scale(zoom, zoom);
        m_mapDisplay->rotate(m_mapRotation);

        // Re-center after rotation
        if (m_mapDisplay->scene()) {
            QRectF sceneRect = m_mapDisplay->scene()->sceneRect();
            if (!sceneRect.isEmpty()) {
                m_mapDisplay->centerOn(sceneRect.center());
            }
        }
    }

    // Only sync rotation to Player Window if sync is enabled
    if (m_syncRotationToPlayer && m_playerWindow) {
        m_playerRotation = m_mapRotation;  // Keep player rotation in sync
        m_playerWindow->setRotation(m_playerRotation);  // Use PlayerWindow's rotation tracking
    }

    updateStatusIndicators();
    QString rotationText = QString("%1°").arg(m_mapRotation);
    statusBar()->showMessage(QString("Map rotated to %1").arg(rotationText), 2000);
}

void MainWindow::toggleRotationSync()
{
    m_syncRotationToPlayer = !m_syncRotationToPlayer;

    // Update the menu action state
    if (m_syncRotationToggleAction) {
        m_syncRotationToggleAction->setChecked(m_syncRotationToPlayer);
    }

    // If sync was just enabled, immediately sync the current DM rotation to player
    if (m_syncRotationToPlayer) {
        syncRotationToPlayer();
        statusBar()->showMessage("Rotation sync enabled - player view now matches DM", 2000);
    } else {
        statusBar()->showMessage("Rotation sync disabled - DM and player views can differ", 2000);
    }

    updateStatusIndicators();
}

void MainWindow::syncRotationToPlayer()
{
    if (!m_playerWindow) {
        statusBar()->showMessage("Player Window not open", 2000);
        return;
    }

    if (m_mapRotation != m_playerRotation) {
        m_playerRotation = m_mapRotation;
        m_playerWindow->setRotation(m_playerRotation);
        statusBar()->showMessage(QString("Player rotation synced to %1°").arg(m_mapRotation), 2000);
    } else {
        statusBar()->showMessage("Player rotation already matches DM", 2000);
    }

    updateStatusIndicators();
}

void MainWindow::rotatePlayerView()
{
    if (!m_playerWindow) {
        statusBar()->showMessage("Player Window not open", 2000);
        return;
    }

    // Rotate player view 90° clockwise
    m_playerRotation = (m_playerRotation + 90) % 360;
    m_playerWindow->setRotation(m_playerRotation);

    updateStatusIndicators();
    statusBar()->showMessage(QString("Player view rotated to %1°").arg(m_playerRotation), 2000);
}

void MainWindow::zoomIn()
{
    if (m_mapDisplay) {
        // Fixed 25% increment for predictable zooming
        qreal currentScale = m_mapDisplay->transform().m11();
        qreal newScale = currentScale * 1.25; // 25% increase

        // Clamp to maximum zoom
        if (newScale <= 5.0) {
            QPointF centerPoint = m_mapDisplay->mapToScene(m_mapDisplay->rect().center());
            m_mapDisplay->resetTransform();
            m_mapDisplay->scale(newScale, newScale);
            m_mapDisplay->centerOn(centerPoint);

            updateZoomStatus();
            statusBar()->showMessage(QString("Zoomed in to %1%").arg(qRound(newScale * 100)), 2000);

            // Update zoom spinner
            if (m_zoomSpinner) {
                int newValue = qRound(newScale * 100);
                m_updatingZoomSpinner = true;
                m_zoomSpinner->setValue(newValue);
                m_updatingZoomSpinner = false;
            }

            // Sync with player window - using syncZoomWithPlayer which is connected to zoomChanged
            syncZoomWithPlayer(newScale);
        }
    }
}

void MainWindow::zoomOut()
{
    if (m_mapDisplay) {
        // Fixed 25% decrement for predictable zooming
        qreal currentScale = m_mapDisplay->transform().m11();
        qreal newScale = currentScale * 0.8; // 20% decrease (inverse of 25% increase)

        // Clamp to minimum zoom
        if (newScale >= 0.1) {
            QPointF centerPoint = m_mapDisplay->mapToScene(m_mapDisplay->rect().center());
            m_mapDisplay->resetTransform();
            m_mapDisplay->scale(newScale, newScale);
            m_mapDisplay->centerOn(centerPoint);

            updateZoomStatus();
            statusBar()->showMessage(QString("Zoomed out to %1%").arg(qRound(newScale * 100)), 2000);

            // Update zoom spinner
            if (m_zoomSpinner) {
                int newValue = qRound(newScale * 100);
                qDebug() << "zoomOut: Current scale =" << newScale << ", spinner should be" << newValue << "%, current value =" << m_zoomSpinner->value();
                m_updatingZoomSpinner = true;
                m_zoomSpinner->setValue(newValue);
                m_updatingZoomSpinner = false;
                qDebug() << "zoomOut: After setValue, spinner is now" << m_zoomSpinner->value() << "%";
            }

            // Sync with player window - using syncZoomWithPlayer which is connected to zoomChanged
            syncZoomWithPlayer(newScale);
        }
    }
}



void MainWindow::updateFogModeIndicator()
{
    if (!m_mapDisplay) return;
    bool hideMode = m_mapDisplay->isFogHideModeEnabled();

    if (m_fogHideToggleAction) {
        m_fogHideToggleAction->setText(hideMode ? "HIDE" : "REVEAL");
        m_fogHideToggleAction->setChecked(hideMode);
    }

    // Update brush/rect button labels
    if (m_fogBrushAction) {
        m_fogBrushAction->setText(hideMode ? "Hide Brush" : "Reveal Brush");
        m_fogBrushAction->setToolTip(hideMode
            ? "<b>Hide Brush</b><br>Paint fog over revealed areas<br><i>Shortcut: H to toggle mode</i>"
            : "<b>Reveal Brush</b><br>Paint to reveal areas<br><i>Shortcut: H to toggle mode</i>");
    }

    if (m_fogRectAction) {
        m_fogRectAction->setText(hideMode ? "Hide Rect" : "Reveal Rect");
        m_fogRectAction->setToolTip(hideMode
            ? "<b>Hide Rectangle</b><br>Drag to cover area with fog<br><i>Shortcut: H to toggle mode</i>"
            : "<b>Reveal Rectangle</b><br>Drag to reveal rectangular area<br><i>Shortcut: H to toggle mode</i>");
    }

    // Color the toggle button: green for reveal, red for hide
    if (auto* btn = qobject_cast<QToolButton*>(m_mainToolBar->widgetForAction(m_fogHideToggleAction))) {
        btn->setStyleSheet(hideMode
            ? DarkTheme::dangerButtonStyle()
            : DarkTheme::successButtonStyle());
    }
}

void MainWindow::toggleFogHideMode()
{
    if (m_mapDisplay) {
        const bool newHideMode = !m_mapDisplay->isFogHideModeEnabled();
        m_mapDisplay->setFogHideModeEnabled(newHideMode);

        // Enable fog mode if hide mode is turned on
        if (newHideMode && !m_mapDisplay->isFogEnabled()) {
            m_mapDisplay->setFogEnabled(true);
        }

        if (newHideMode) {
            statusBar()->showMessage("Fog Hide Mode: painting will cover areas with fog", 3000);
        } else {
            statusBar()->showMessage("Fog Reveal Mode: painting will reveal areas through fog", 3000);
        }

        updateFogModeIndicator();
        updateStatusIndicators();
    }
}

void MainWindow::toggleFogRectangleMode()
{
    if (m_mapDisplay) {
        const bool newRectMode = !m_mapDisplay->isFogRectangleModeEnabled();
        m_mapDisplay->setFogRectangleModeEnabled(newRectMode);

        // Update the menu action's checked state
        if (m_fogRectangleModeAction) {
            m_fogRectangleModeAction->setChecked(newRectMode);
        }

        // Enable fog mode if rectangle mode is turned on
        if (newRectMode && !m_mapDisplay->isFogEnabled()) {
            m_mapDisplay->setFogEnabled(true);
        }

        // Turn off other modes when fog rectangle mode is enabled
        if (newRectMode) {
            // Turn off fog hide mode if fog rectangle mode is enabled
            if (m_mapDisplay->isFogHideModeEnabled()) {
                m_mapDisplay->setFogHideModeEnabled(false);
            }
            statusBar()->showMessage("Rectangle fog mode enabled - click and drag to reveal/hide rectangular areas", 3000);
        } else {
            statusBar()->showMessage("Rectangle fog mode disabled - using brush fog mode", 3000);
        }

        updateStatusIndicators();

        // Sync with player window if open
        if (m_playerWindow) {
            m_playerWindow->forceRefresh();
        }
    }
}

void MainWindow::togglePlayerViewMode()
{
    m_playerViewModeEnabled = !m_playerViewModeEnabled;

    if (m_mapDisplay && m_mapDisplay->getFogOverlay()) {
        // Update the fog overlay to use player view mode override
        m_mapDisplay->getFogOverlay()->setPlayerViewMode(m_playerViewModeEnabled);

        if (m_playerViewModeEnabled) {
            statusBar()->showMessage("Player view mode enabled - DM sees fog exactly as players do", 3000);
        } else {
            statusBar()->showMessage("Player view mode disabled - DM sees fog with configured transparency", 3000);
        }

        updateStatusIndicators();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Handle privacy shield shortcuts first (highest priority)
    // Note: Ctrl+B shortcut disabled; privacy/intermission feature still active via activateBlackout()
    if (event->key() == Qt::Key_B && event->modifiers() == Qt::NoModifier) {
        // B = Blackout
        activateBlackout();
        event->accept();
        return;
    }

    // Handle unified tool system (NO number key shortcuts per CLAUDE.md)
    if (event->modifiers() == Qt::NoModifier) {
        switch (event->key()) {
        case Qt::Key_Escape:
            if (m_toolManager && m_toolManager->handleEscapeKey()) {
                event->accept();
                return;
            }
            break;
        // Key_0 removed - use '/' for fit-to-screen (CLAUDE.md 3.6)
        }
    }

    // Call parent implementation for other keys
    QMainWindow::keyPressEvent(event);
}

void MainWindow::showGridInfo()
{
    if (m_gridController) {
        m_gridController->showGridInfo();
    }
}

void MainWindow::setStandardGrid()
{
    if (m_gridController) {
        m_gridController->setStandardGrid();
        updateStatusIndicators();
        if (m_playerWindowController) {
            m_playerWindowController->updatePlayerWindow();
        }
    }
}


void MainWindow::onTabChanged(int index)
{
    if (m_tabsController) m_tabsController->setCurrentIndex(index);
}

void MainWindow::onTabCloseRequested(int index)
{
    if (m_tabsController) m_tabsController->closeIndex(index);
}

void MainWindow::switchToTab(int index)
{
    if (m_tabsController) m_tabsController->setCurrentIndex(index);
}

void MainWindow::closeTab(int index)
{
    if (m_tabsController) m_tabsController->closeIndex(index);
}


void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    SettingsManager::instance().saveWindowGeometry("MainWindow", geometry());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    SettingsManager::instance().saveWindowGeometry("MainWindow", geometry());
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // CRITICAL: Activate window and grab keyboard focus when shown
    // This ensures shortcuts work immediately without requiring the user to click inside
    activateWindow();
    raise();
    setFocus();
}

void MainWindow::toggleGridType()
{
    if (m_gridController) {
        m_gridController->toggleGridType();
        if (m_playerWindowController) {
            m_playerWindowController->updatePlayerWindow();
        }
    }
}

// ====================================================================================
// UNIFIED FOG TOOL MODE SYSTEM IMPLEMENTATION
// ====================================================================================

void MainWindow::setupFogToolModeSystem()
{
    // SIMPLIFIED per CLAUDE.md: No fog mode buttons needed
    // The fog tool uses modifiers for different operations:
    // - Click = reveal
    // - Alt+Click = hide
    // - Shift+Drag = rectangle
    // This is all handled in MapDisplay mouse event handlers

    updateFogToolModeUI();
}

void MainWindow::setFogToolMode(FogToolMode mode)
{
    if (m_fogToolMode == mode) {
        return;  // No change needed
    }

    m_fogToolMode = mode;
    if (m_fogToolsController) {
        m_fogToolsController->setMode(mode);
    }

    // Reset fog sub-modes — unified fog mode uses modifier keys at paint time
    if (m_mapDisplay) {
        m_mapDisplay->setFogHideModeEnabled(false);
        m_mapDisplay->setFogRectangleModeEnabled(false);

        // Do NOT auto-enable fog; respect the user's explicit fog toggle

        // Update cursor to reflect the new tool mode
        m_mapDisplay->updateToolCursor();
    }

    // CRITICAL FIX: Sync fog tool mode to PlayerWindow for cursor synchronization
    if (m_playerWindow && m_playerWindow->isVisible()) {
        m_playerWindow->updateToolCursor();
    }

    updateFogToolModeUI();
    updateFogToolModeStatus();
    updateContextualToolbarControls();  // Update toolbar controls when fog tool mode changes

    // Update Active Tool overlay chip with shortcut hints and brush size
    if (auto* toolOverlay = m_mapDisplay->findChild<ToolOverlayWidget*>("toolOverlay")) {
        QString modeText = getFogToolModeText(m_fogToolMode);
        QString hints = "(R/H - Shift+R/H)";
        int brush = m_mapDisplay ? m_mapDisplay->getFogBrushSize() : 0;
        toolOverlay->setText(QString("Tool: %1 %2 — %3px").arg(modeText, hints).arg(brush));
    }
    updateStatusIndicators();

    // Sync with player window
    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }
}

void MainWindow::onExposureChanged(int value)
{
    if (!m_mapDisplay) {
        return;
    }

    // Convert slider value (1-200) to exposure (0.01-2.0)
    float exposure = value / 100.0f;

    // Update label
    if (m_exposureLabel) {
        m_exposureLabel->setText(QString("Exposure: %1").arg(exposure, 0, 'f', 2));
    }

    statusBar()->showMessage(QString("Exposure set to %1").arg(exposure, 0, 'f', 2), 2000);
}


void MainWindow::updateFogToolModeUI()
{
    // SIMPLIFIED per CLAUDE.md: No fog mode buttons to update
    // The unified fog tool doesn't have UI buttons - it uses modifiers
    // All fog mode actions are nullptr, so nothing to update here
}

void MainWindow::updateFogToolModeStatus()
{
    QString modeText = getFogToolModeText(m_fogToolMode);
    QString instructions = getFogToolModeInstructions(m_fogToolMode);

    statusBar()->showMessage(QString("Fog Tool: %1 - %2").arg(modeText, instructions), 5000);
}

QString MainWindow::getFogToolModeText(FogToolMode mode) const
{
    switch (mode) {
        case FogToolMode::UnifiedFog:
            return "Fog (Alt=Hide, Shift=Rect)";
        default:
            return "Unknown";
    }
}

QString MainWindow::getFogToolModeInstructions(FogToolMode mode) const
{
    switch (mode) {
        case FogToolMode::UnifiedFog:
            return "Click=Reveal, Alt+Click=Hide, Shift=Rectangle, Double-click=Clear visible, [/]=Size";
        default:
            return "";
    }
}

// ====================================================================================
// UNDO/REDO SYSTEM IMPLEMENTATION
// ====================================================================================

void MainWindow::undoFogChange()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) {
        return;
    }

    FogOfWar* fogOverlay = m_mapDisplay->getFogOverlay();
    if (fogOverlay->canUndo()) {
        fogOverlay->undo();

        // Update undo/redo button states
        updateUndoRedoButtons();

        // Sync with player window
        if (m_playerWindow) {
            m_playerWindow->forceRefresh();
        }

        statusBar()->showMessage("Fog painting undone", 2000);
    }
}

void MainWindow::redoFogChange()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) {
        return;
    }

    FogOfWar* fogOverlay = m_mapDisplay->getFogOverlay();
    if (fogOverlay->canRedo()) {
        fogOverlay->redo();

        // Update undo/redo button states
        updateUndoRedoButtons();

        // Sync with player window
        if (m_playerWindow) {
            m_playerWindow->forceRefresh();
        }

        statusBar()->showMessage("Fog painting redone", 2000);
    }
}

void MainWindow::updateUndoRedoButtons()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay() || !m_undoAction || !m_redoAction) {
        return;
    }

    FogOfWar* fogOverlay = m_mapDisplay->getFogOverlay();

    // Update undo action
    bool canUndo = fogOverlay->canUndo();
    m_undoAction->setEnabled(canUndo);

    // Update redo action
    bool canRedo = fogOverlay->canRedo();
    m_redoAction->setEnabled(canRedo);
}

void MainWindow::toggleLighting()
{
    if (!m_mapDisplay) {
        return;
    }

    bool currentlyEnabled = m_mapDisplay->isLightingEnabled();
    m_mapDisplay->setLightingEnabled(!currentlyEnabled);

    // Update lighting controls
    updateLightingControls();
    
    // Show toast notification
    if (auto* toast = ToastNotification::instance(this)) {
        QString message = !currentlyEnabled ? "Dynamic lighting enabled" : "Dynamic lighting disabled";
        toast->showMessage(message, ToastNotification::Type::Info, 2000);
    }

    // Sync with player window
    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }

    // Show status message
    QString status = currentlyEnabled ? "Lighting disabled" : "Lighting enabled";
    statusBar()->showMessage(status, 2000);
}

void MainWindow::setTimeOfDay(int timeOfDay)
{
    if (!m_mapDisplay) {
        return;
    }

    // Set time of day on the lighting overlay
    m_mapDisplay->setTimeOfDay(timeOfDay);

    // Update lighting controls
    updateLightingControls();

    // Sync with player window
    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }

    // Show status message with time of day name
    QStringList timeNames = {"Dawn", "Day", "Dusk", "Night"};
    QString timeName = (timeOfDay >= 0 && timeOfDay < timeNames.size()) ? timeNames[timeOfDay] : "Unknown";
    statusBar()->showMessage(QString("Time of day set to %1").arg(timeName), 2000);
}

void MainWindow::onLightingIntensityChanged(int value)
{
    if (!m_mapDisplay) {
        return;
    }

    // Convert slider value (0-100) to intensity (0.0-1.0)
    qreal intensity = value / 100.0;
    m_mapDisplay->setLightingIntensity(intensity);

    // Update label if it exists
    if (m_lightingIntensityLabel) {
        m_lightingIntensityLabel->setText(QString("Intensity: %1%").arg(value));
    }

    // Sync with player window
    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }
}

// Point light controls implementation
void MainWindow::togglePointLightPlacement()
{
    // Point light feature removed - no longer supported
}

void MainWindow::onAmbientLightChanged(int value)
{
    if (!m_mapDisplay) {
        return;
    }

    // Convert slider value (0-100) to level (0.0-1.0)
    qreal level = value / 100.0;
    m_mapDisplay->setAmbientLightLevel(level);

    // Update label if it exists
    if (m_ambientLightLabel) {
        m_ambientLightLabel->setText(QString("Ambient: %1%").arg(value));
    }

    // Sync with player window
    if (m_playerWindow) {
        m_playerWindow->forceRefresh();
    }
}

void MainWindow::clearAllPointLights()
{
    // Point light feature removed - no longer supported
}

void MainWindow::showPointLightProperties(const QUuid& lightId)
{
    if (!m_mapDisplay || lightId.isNull()) {
        return;
    }

    PointLightSystem* lightSystem = m_mapDisplay->getPointLightSystem();
    if (!lightSystem) {
        return;
    }

    const PointLight* light = lightSystem->getLight(lightId);
    if (!light) {
        return;
    }

    // Create and show the edit dialog
    LightEditDialog dialog(this);
    dialog.setLight(*light);

    if (dialog.exec() == QDialog::Accepted) {
        // Apply changes to the light
        PointLight editedLight = dialog.getLight();
        lightSystem->updateLight(lightId, editedLight);
        qDebug() << "MainWindow: Updated light" << lightId << "- name:" << editedLight.name;
    }
}

void MainWindow::updateLightingControls()
{
    if (!m_mapDisplay) {
        return;
    }

    bool lightingEnabled = m_mapDisplay->isLightingEnabled();

    // Update lighting action state
    if (m_lightingAction) {
        m_lightingAction->setChecked(lightingEnabled);
    }

    // Update time of day action states and enabled state
    int currentTimeOfDay = m_mapDisplay->getTimeOfDay();

    if (m_dawnAction) {
        m_dawnAction->setEnabled(lightingEnabled);
        m_dawnAction->setChecked(lightingEnabled && currentTimeOfDay == 0);
    }

    if (m_dayAction) {
        m_dayAction->setEnabled(lightingEnabled);
        m_dayAction->setChecked(lightingEnabled && currentTimeOfDay == 1);
    }

    if (m_duskAction) {
        m_duskAction->setEnabled(lightingEnabled);
        m_duskAction->setChecked(lightingEnabled && currentTimeOfDay == 2);
    }

    if (m_nightAction) {
        m_nightAction->setEnabled(lightingEnabled);
        m_nightAction->setChecked(lightingEnabled && currentTimeOfDay == 3);
    }

    // Update time of day button states (if they exist)
    if (m_timeOfDayButtonGroup) {
        // Enable/disable time of day controls based on lighting state
        QList<QAbstractButton*> buttons = m_timeOfDayButtonGroup->buttons();
        for (QAbstractButton* button : buttons) {
            button->setEnabled(lightingEnabled);
        }
    }

    // Enable/disable intensity slider
    if (m_lightingIntensitySlider) {
        m_lightingIntensitySlider->setEnabled(lightingEnabled);
    }

    // Update time of day label
    if (m_timeOfDayLabel) {
        m_timeOfDayLabel->setEnabled(lightingEnabled);
    }

    // Update intensity label
    if (m_lightingIntensityLabel) {
        m_lightingIntensityLabel->setEnabled(lightingEnabled);
    }
}



// Helper methods for state synchronization
void MainWindow::updateAllMenuStates()
{
    // Update grid state
    if (m_toggleGridAction) {
        m_toggleGridAction->setChecked(m_mapDisplay && m_mapDisplay->isGridEnabled());
    }

    // Update lighting states
    updateLightingControls();
}

void MainWindow::toggleDebugConsole()
{
    // Create debug console on first use to avoid recursive initialization
    if (!m_debugConsoleWidget) {
        m_debugConsoleWidget = new DebugConsoleWidget(this);
        m_debugConsoleWidget->setWindowFlags(Qt::Window);
    }

    if (m_debugConsoleWidget->isVisible()) {
        m_debugConsoleWidget->hide();
        DebugConsole::info("Debug Console closed", "UI");
    } else {
        m_debugConsoleWidget->show();
        m_debugConsoleWidget->raise();
        m_debugConsoleWidget->activateWindow();
        DebugConsole::info("Debug Console opened", "UI");
    }
}

void MainWindow::toggleMapBrowser()
{
    if (!m_mapBrowserWidget) {
        return;
    }

    if (m_mapBrowserWidget->isVisible()) {
        m_mapBrowserWidget->hide();
        if (m_mapBrowserAction) {
            m_mapBrowserAction->setChecked(false);
        }
    } else {
        m_mapBrowserWidget->show();
        m_mapBrowserWidget->refreshRecentFiles();
        if (m_mapBrowserAction) {
            m_mapBrowserAction->setChecked(true);
        }
    }
}

void MainWindow::toggleAtmosphereToolbox()
{
    if (!m_atmosphereToolbox) {
        return;
    }

    if (m_atmosphereToolbox->isVisible()) {
        m_atmosphereToolbox->hide();
        if (m_atmosphereToolboxAction) {
            m_atmosphereToolboxAction->setChecked(false);
        }
    } else {
        m_atmosphereToolbox->show();
        if (m_atmosphereToolboxAction) {
            m_atmosphereToolboxAction->setChecked(true);
        }
    }
}

void MainWindow::showKeyboardShortcuts()
{
    if (!m_shortcutsOverlay) {
        m_shortcutsOverlay = new KeyboardShortcutsOverlay(this);
    }
    m_shortcutsOverlay->showOverlay();
}

void MainWindow::showQuickStartGuide()
{
    QString guideText =
        "<h3>Quick Start Guide</h3>"
        "<p><b>1. Connect TV as second display</b><br>"
        "Set up your TV or second monitor as an extended display.</p>"
        "<p><b>2. Launch Crit VTT</b><br>"
        "The application will open with the main control window.</p>"
        "<p><b>3. Drag map onto main window</b><br>"
        "Drop any image file or VTT file onto the main window to load it.</p>"
        "<p><b>4. Player window appears automatically</b><br>"
        "The player view opens automatically on your TV/second display.</p>"
        "<p><b>5. Use fog tools to hide/reveal areas</b><br>"
        "Press F to enable Fog of War, then paint to reveal areas.<br>"
        "Press H to toggle between Reveal and Hide modes.<br>"
        "Use [ and ] to change brush size.</p>"
        "<p><i>Just maps on a TV - that's the entire scope!</i></p>";

    QMessageBox::information(this, "Quick Start Guide", guideText);
}

void MainWindow::showAboutDialog()
{
    QString aboutText =
        QString("<h3>Crit VTT v%1</h3>").arg(APP_VERSION) +
        "<p><b>Atmospheric maps for in-person tabletop gaming</b></p>"
        "<p>Display maps on your TV with fog of war, lighting, weather, "
        "and ambient sound for immersive game nights.</p>"
        "<p>Copyright &copy; 2024-2026 Crit VTT<br>"
        "Licensed under the MIT License</p>";

    QMessageBox::about(this, "About Crit VTT", aboutText);
}

void MainWindow::activateBlackout()
{
    if (m_playerWindowController && m_playerWindowController->getPlayerWindow()) {
        PlayerWindow* playerWindow = m_playerWindowController->getPlayerWindow();
        playerWindow->activateBlackout();

        // Update privacy status indicator
        updatePrivacyStatusIndicator(true, "Blackout");

        // Show status message in main window
        statusBar()->showMessage("Privacy Blackout Activated - Press Escape on player window to exit", 5000);

        // Show toast notification
        if (auto* toast = ToastNotification::instance(this)) {
            toast->showMessage("Blackout Activated", ToastNotification::Type::Warning, 2000);
        }
    }
}

void MainWindow::activateIntermission()
{
    if (m_playerWindowController && m_playerWindowController->getPlayerWindow()) {
        PlayerWindow* playerWindow = m_playerWindowController->getPlayerWindow();
        playerWindow->activateIntermission();

        // Update privacy status indicator
        updatePrivacyStatusIndicator(true, "Intermission");

        // Show status message in main window
        statusBar()->showMessage("Intermission Screen Active - Press Escape on player window to exit", 5000);

        // Show toast notification
        if (auto* toast = ToastNotification::instance(this)) {
            toast->showMessage("Intermission Active", ToastNotification::Type::Info, 2000);
        }
    }
}

void MainWindow::deactivatePrivacyMode()
{
    if (m_playerWindowController && m_playerWindowController->getPlayerWindow()) {
        PlayerWindow* playerWindow = m_playerWindowController->getPlayerWindow();
        playerWindow->deactivatePrivacyMode();

        // Update privacy status indicator
        updatePrivacyStatusIndicator(false);

        // Show status message in main window
        statusBar()->showMessage("Privacy mode deactivated", 2000);

        // Show toast notification
        if (auto* toast = ToastNotification::instance(this)) {
            toast->showMessage("Privacy Mode Off", ToastNotification::Type::Success, 2000);
        }
    }
}

void MainWindow::updatePrivacyStatusIndicator(bool privacyActive, const QString& modeText)
{
    if (!m_privacyStatusLabel) return;

    if (privacyActive) {
        QString displayText = modeText.isEmpty() ? "Privacy" : modeText;
        m_privacyStatusLabel->setText(QString("%1").arg(displayText));
        m_privacyStatusLabel->show();
    } else {
        m_privacyStatusLabel->hide();
    }
}
