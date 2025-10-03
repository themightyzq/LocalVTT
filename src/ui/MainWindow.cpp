#include "ui/MainWindow.h"
#include <iostream>
#include "graphics/MapDisplay.h"
#include "utils/ActionRegistry.h"
#include "utils/AnimationHelper.h"
#include "utils/SecureWindowRegistry.h"
#include "ui/PlayerWindow.h"
#include "graphics/FogOfWar.h"
#include "graphics/GridOverlay.h"
#include "utils/SettingsManager.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/ToolboxWidget.h"
#include "ui/ToastNotification.h"
#include "ui/LoadingOverlay.h"
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
#include "controllers/ToolManager.h"
#include "utils/ToolType.h"
#include "ui/widgets/ToolStatusWidget.h"
#include "utils/MapSession.h"
#include "graphics/ToolOverlayWidget.h"
#include "graphics/LightingOverlay.h"
#include "graphics/SceneManager.h"
#include "opengl/OpenGLMapDisplay.h"
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
#include <QMimeData>
#include <QApplication>
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
#include <QTimer>
#include <QDir>
#include <QKeyEvent>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QSlider>
#include <QSpinBox>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QLabel>
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
    , m_tabWidget(nullptr)
    , m_recentFilesMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_progressDialog(nullptr)
    , m_loadingOverlay(nullptr)
    , m_dropOverlay(nullptr)
    , m_dropAnimation(nullptr)
    , m_clearRecentAction(nullptr)
    , m_recentFilesController(nullptr)
    , m_gridEnabled(true)
    , m_fogEnabled(false)
    , m_isDragging(false)
    , m_updatingZoomSpinner(false)
    , m_fogToolMode(FogToolMode::UnifiedFog)
    , m_fogHideModeEnabled(false)
    , m_fogRectangleModeEnabled(false)
    , m_playerViewModeEnabled(false)
    , m_statusContainer(nullptr)
    , m_gridStatusLabel(nullptr)
    , m_fogStatusLabel(nullptr)
    , m_playerViewStatusLabel(nullptr)
    , m_zoomStatusLabel(nullptr)
    , m_privacyStatusLabel(nullptr)
    , m_fogAutosaveController(nullptr)
    , m_tabsController(nullptr)
    , m_zoomToolBar(nullptr)
    , m_fitToScreenAction(nullptr)
    , m_zoomInAction(nullptr)
    , m_zoomOutAction(nullptr)
    , m_zoomPercentageLabel(nullptr)
    , m_gridSizeSpinner(nullptr)
    , m_gridSizeLabel(nullptr)
    , m_zoomSpinner(nullptr)
    , m_fogBrushSlider(nullptr)
    , m_fogBrushLabel(nullptr)
    , m_gmOpacitySlider(nullptr)
    , m_gmOpacityLabel(nullptr)
    , m_resetFogAction(nullptr)
    , m_brushSizeDebounceTimer(nullptr)
    , m_gridSizeDebounceTimer(nullptr)
    , m_controlStripDock(nullptr)
    , m_fogToolButtonGroup(nullptr)
    , m_revealCircleAction(nullptr)
    , m_hideCircleAction(nullptr)
    , m_revealRectangleAction(nullptr)
    , m_hideRectangleAction(nullptr)
    , m_revealFeatheredAction(nullptr)
    , m_hideFeatheredAction(nullptr)
    , m_drawPenAction(nullptr)
    , m_drawEraserAction(nullptr)
    , m_fogToolModeLabel(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_toggleGridAction(nullptr)
    , m_playerWindowToggleAction(nullptr)
    , m_fogBrushAction(nullptr)
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
    , m_debugConsoleWidget(nullptr)
    , m_debugConsoleAction(nullptr)
    , m_toolStatusWidget(nullptr)
    , m_gridController(nullptr)
    , m_lightingController(nullptr)
    , m_viewZoomController(nullptr)
    , m_playerWindowController(nullptr)
    , m_fogToolsController(nullptr)
{
    std::cerr << "MainWindow constructor starting..." << std::endl;
    std::cerr << "MainWindow: Member initialization complete" << std::endl;
    std::cerr.flush();

    // CRITICAL: Remove try-catch blocks that hide errors - we need to see what's failing
    // Set initial window properties
    setWindowTitle("LocalVTT - Initializing...");
    resize(1200, 800);
    std::cerr << "MainWindow: Basic properties set" << std::endl;

    // Initialize ActionRegistry first (many components depend on it)
    std::cerr << "MainWindow: Creating ActionRegistry..." << std::endl;
    m_actionRegistry = new ActionRegistry(this);
    if (!m_actionRegistry) {
        std::cerr << "FATAL: Failed to create ActionRegistry!" << std::endl;
        abort();  // Force crash with core dump for debugging
    }
    std::cerr << "MainWindow: ActionRegistry created successfully" << std::endl;

    // Load settings and window geometry
    std::cerr << "MainWindow: Loading settings..." << std::endl;
    SettingsManager& settings = SettingsManager::instance();
    QRect defaultGeometry(100, 100, 1200, 800);
    QRect savedGeometry = settings.loadWindowGeometry("MainWindow", defaultGeometry);
    setGeometry(savedGeometry);
    std::cerr << "MainWindow: Settings loaded" << std::endl;

    // MEMORY OPTIMIZATION: Defer heavy UI initialization
    // Only create bare minimum UI components initially
    std::cerr << "MainWindow: Setting up minimal UI..." << std::endl;
    setupMinimalUI();  // Create only essential components
    std::cerr << "MainWindow: Minimal UI completed" << std::endl;

    std::cerr << "MainWindow: Deferring heavy initialization..." << std::endl;
    // Defer creation of controllers and managers until needed
    QTimer::singleShot(10, this, [this]() {
        setupDeferredComponents();
    });

    std::cerr << "MainWindow: Setting up core actions..." << std::endl;
    setupActions();
    std::cerr << "MainWindow: setupActions completed" << std::endl;

    std::cerr << "MainWindow: Creating menus..." << std::endl;
    createMenus();
    std::cerr << "MainWindow: createMenus completed" << std::endl;

    // MEMORY OPTIMIZATION: Defer toolbox and fog controller creation
    std::cerr << "MainWindow: Deferring toolbox creation" << std::endl;

    // Setup fog tool mode system BEFORE creating toolbar (toolbar needs these actions)
    std::cerr << "MainWindow: Setting up fog tool mode system..." << std::endl;
    setupFogToolModeSystem();
    std::cerr << "MainWindow: Fog tool mode system setup completed" << std::endl;

    std::cerr << "MainWindow: Creating toolbar..." << std::endl;
    createToolbar();
    std::cerr << "MainWindow: createToolbar completed" << std::endl;

    std::cerr << "MainWindow: Creating zoom toolbar..." << std::endl;
    createZoomToolbar();
    std::cerr << "MainWindow: createZoomToolbar completed" << std::endl;

    std::cerr << "MainWindow: Setting up status bar..." << std::endl;
    setupStatusBar();
    std::cerr << "MainWindow: setupStatusBar completed" << std::endl;

    // Initialize controllers that depend on UI components
    // Note: ToolManager and FogToolsController will be created after MapDisplay
    // is created in loadMapFile() to avoid null pointer issues

    // MEMORY OPTIMIZATION: Defer controller creation until needed
    std::cerr << "MainWindow: Deferring controller creation..." << std::endl;
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
    std::cerr << "MainWindow: Essential controllers created" << std::endl;

    // Update UI states
    std::cerr << "MainWindow: Updating UI states..." << std::endl;
    updateRecentFilesMenu();
    updateUndoRedoButtons();
    updateAllMenuStates();
    std::cerr << "MainWindow: UI states updated" << std::endl;

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

    SecureWindowRegistry::instance().registerWindow(this, SecureWindowRegistry::MainWindow);

    setAcceptDrops(true);
    setWindowTitle("LocalVTT - Ready");
    std::cerr << "MainWindow constructor completed successfully!" << std::endl;
    std::cerr.flush();

    // Add debug to confirm window state
    std::cerr << "MainWindow isVisible: " << isVisible() << std::endl;
    std::cerr << "MainWindow geometry: " << geometry().x() << "," << geometry().y()
              << " " << geometry().width() << "x" << geometry().height() << std::endl;
    std::cerr.flush();
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

void MainWindow::setupMinimalUI()
{
    // CRITICAL FIX: Create MapDisplay and ToolManager eagerly to enable tool switching
    std::cerr << "setupMinimalUI: Creating essential components with MapDisplay..." << std::endl;
    std::cerr.flush();

    // CRITICAL: Remove try-catch blocks to see actual errors
    // Create central widget with tab system
    QWidget* centralWidget = new QWidget(this);
    if (!centralWidget) {
        std::cerr << "FATAL: Failed to create central widget!" << std::endl;
        abort();
    }

    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    if (!layout) {
        std::cerr << "FATAL: Failed to create layout!" << std::endl;
        abort();
    }
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // CRITICAL FIX: Create MapDisplay and tab widget eagerly
    // This ensures tool switching works immediately, even without a loaded map
    std::cerr << "setupMinimalUI: Creating MapDisplay eagerly for tool switching..." << std::endl;

    // Create MapDisplay immediately
    m_mapDisplay = new MapDisplay(this);
    if (!m_mapDisplay) {
        std::cerr << "FATAL: Failed to create MapDisplay!" << std::endl;
        abort();
    }
    m_mapDisplay->setMainWindow(this);

    // Create tab widget with MapDisplay
    m_tabWidget = new QTabWidget(this);
    if (!m_tabWidget) {
        std::cerr << "FATAL: Failed to create tab widget!" << std::endl;
        abort();
    }
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);

    // Add an initial empty tab
    m_tabWidget->addTab(m_mapDisplay, "Empty Map");
    layout->addWidget(m_tabWidget);

    // Create FogToolsController early (needed for fog tool)
    m_fogToolsController = new FogToolsController(this);

    // Create ToolManager immediately so tool switching is available
    std::cerr << "setupMinimalUI: Creating ToolManager..." << std::endl;
    m_toolManager = new ToolManager(this, m_mapDisplay, this);

    // Connect tool switching signal from MapDisplay to ToolManager
    connect(m_mapDisplay, &MapDisplay::toolSwitchRequested,
            m_toolManager, &ToolManager::setActiveTool);

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

    std::cerr << "setupMinimalUI: MapDisplay and ToolManager created successfully" << std::endl;

    setCentralWidget(centralWidget);
    std::cerr << "setupUI: Central widget set successfully" << std::endl;
    std::cerr.flush();

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

    std::cerr << "setupMinimalUI: Minimal UI setup completed successfully" << std::endl;
    std::cerr.flush();
}

void MainWindow::setupDeferredComponents()
{
    std::cerr << "setupDeferredComponents: Creating deferred components..." << std::endl;

    // These components can be created after initial window is shown
    // They're not needed immediately for basic functionality

    // Defer controller creation - create on first use
    // m_gridController - create when grid is first toggled
    // m_lightingController - create when lighting is first enabled
    // m_viewZoomController - create when zoom is first used
    // m_playerWindowController - create when player window is opened
    // m_fogToolsController - create when fog is first enabled

    std::cerr << "setupDeferredComponents: Deferred creation complete" << std::endl;
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

    // SIMPLIFIED per CLAUDE.md: No fog mode actions needed
    // The unified fog tool uses modifiers:
    // - Click = reveal
    // - Alt+Click = hide
    // - Shift+Drag = rectangle
    // All actions are set to nullptr to prevent crashes
    m_revealCircleAction = nullptr;
    m_hideCircleAction = nullptr;
    m_revealRectangleAction = nullptr;
    m_hideRectangleAction = nullptr;
    m_revealFeatheredAction = nullptr;
    m_hideFeatheredAction = nullptr;
    m_drawPenAction = nullptr;
    m_drawEraserAction = nullptr;

    // Window actions
    m_playerWindowToggleAction = getOrCreateAction("window_player");
    if (m_playerWindowToggleAction) connect(m_playerWindowToggleAction, &QAction::triggered, this, &MainWindow::togglePlayerWindow);

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
        std::cerr << "ERROR: ActionRegistry not initialized in getOrCreateAction!" << std::endl;
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

    m_fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    m_fileMenu->addAction(exitAction);

    // No Edit menu - Undo/Redo available via keyboard shortcuts only

    // VIEW MENU - Simplified
    m_viewMenu = menuBar()->addMenu("&View");

    // Grid toggle
    QAction* gridToggleAction = new QAction("Toggle &Grid", this);
    gridToggleAction->setShortcut(QKeySequence("G"));
    gridToggleAction->setCheckable(true);
    gridToggleAction->setChecked(m_gridEnabled);
    connect(gridToggleAction, &QAction::triggered, this, &MainWindow::toggleGrid);
    m_viewMenu->addAction(gridToggleAction);
    m_toggleGridAction = gridToggleAction;

    // Fog toggle
    QAction* fogToggleAction = getOrCreateAction("fog_toggle");
    if (fogToggleAction) {
        fogToggleAction->setChecked(m_fogEnabled);
        m_viewMenu->addAction(fogToggleAction);
    }

    // Rectangle fog tool toggle
    QAction* fogRectangleAction = new QAction("&Rectangle Fog Tool", this);
    fogRectangleAction->setShortcut(QKeySequence("R"));
    fogRectangleAction->setCheckable(true);
    fogRectangleAction->setChecked(m_fogRectangleModeEnabled);
    connect(fogRectangleAction, &QAction::triggered, this, &MainWindow::toggleFogRectangleMode);
    m_viewMenu->addAction(fogRectangleAction);
    m_fogRectangleModeAction = fogRectangleAction;

    m_viewMenu->addSeparator();

    // Player Window
    QAction* playerWindowAction = new QAction("&Player Window", this);
    playerWindowAction->setShortcut(QKeySequence("P"));
    playerWindowAction->setCheckable(true);
    connect(playerWindowAction, &QAction::triggered, this, &MainWindow::togglePlayerWindow);
    m_viewMenu->addAction(playerWindowAction);

    m_viewMenu->addSeparator();

    // Zoom controls
    QAction* fitAction = new QAction("&Fit to Screen", this);
    fitAction->setShortcut(QKeySequence("0"));
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

    // Premium toolbar styling with subtle gradient and shadows
    m_mainToolBar->setStyleSheet(R"(
        QToolBar {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                        stop: 0 #2a2a2a, stop: 1 #242424);
            border: none;
            border-bottom: 1px solid #1a1a1a;
            padding: 6px;
            spacing: 8px;
        }
        QToolBar::separator {
            background: rgba(255, 255, 255, 0.08);
            width: 1px;
            margin: 4px 8px;
        }
        QToolButton {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            padding: 10px;
            min-width: 36px;
            min-height: 36px;
            color: #E0E0E0;
            font-size: 13px;
            font-weight: 500;
        }
        QToolButton:hover:enabled {
            background: rgba(74, 144, 226, 0.15);
            border-color: rgba(74, 144, 226, 0.3);
        }
        QToolButton:pressed:enabled {
            background: rgba(74, 144, 226, 0.25);
        }
        QToolButton:checked {
            background: rgba(74, 144, 226, 0.3);
            border-color: #4A90E2;
            color: white;
        }
        QToolButton:disabled {
            background: rgba(255, 255, 255, 0.02);
            border-color: rgba(255, 255, 255, 0.05);
            color: rgba(224, 224, 224, 0.3);
        }
    )");

    // SECTION 1: File Operations
    // Load Map button with premium styling
    QIcon openIcon(":/icons/open_map.svg");
    QAction* loadAction = new QAction(openIcon, "Load Map", this);
    loadAction->setToolTip("<b>Load Map</b><br>Open a new map image or VTT file<br><i>Shortcut: Ctrl+O</i>");
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadMap);
    m_mainToolBar->addAction(loadAction);

    m_mainToolBar->addSeparator();

    // SECTION 2: Fog Controls
    // Fog Mode Toggle (Master switch)
    QAction* fogModeAction = new QAction(QIcon(":/icons/fog.svg"), "Fog Mode", this);
    fogModeAction->setCheckable(true);
    fogModeAction->setChecked(false);  // Default OFF
    fogModeAction->setToolTip("<b>Fog Mode</b><br>Enable Fog of War<br>Map goes black until revealed<br><i>Shortcut: F</i>");
    fogModeAction->setShortcut(QKeySequence("F"));
    connect(fogModeAction, &QAction::toggled, this, &MainWindow::toggleFogOfWar);
    m_mainToolBar->addAction(fogModeAction);

    // Reveal Brush Tool (disabled initially)
    m_fogBrushAction = new QAction(QIcon(":/icons/fog_brush.svg"), "Reveal Brush", this);
    m_fogBrushAction->setCheckable(true);
    m_fogBrushAction->setEnabled(false);  // Disabled until Fog Mode ON
    m_fogBrushAction->setToolTip("<b>Reveal Brush</b><br>Circle brush tool<br>Click and drag to reveal areas");
    connect(m_fogBrushAction, &QAction::triggered, [this]() {
        if (m_toolManager) {
            m_toolManager->setActiveTool(ToolType::FogBrush);
            statusBar()->showMessage("Reveal Brush active - Click/drag to reveal areas", 3000);
            updateContextualToolbarControls();  // Update contextual controls
        }
    });
    m_mainToolBar->addAction(m_fogBrushAction);

    // Reveal Rectangle Tool (disabled initially)
    m_fogRectAction = new QAction(QIcon(":/icons/fog_rect.svg"), "Reveal Rectangle", this);
    m_fogRectAction->setCheckable(true);
    m_fogRectAction->setEnabled(false);  // Disabled until Fog Mode ON
    m_fogRectAction->setToolTip("<b>Reveal Rectangle</b><br>Rectangle selection tool<br>Drag to reveal rectangular areas");
    connect(m_fogRectAction, &QAction::triggered, [this]() {
        if (m_toolManager) {
            m_toolManager->setActiveTool(ToolType::FogRectangle);
            statusBar()->showMessage("Reveal Rectangle active - Drag to reveal rectangular area", 3000);
            updateContextualToolbarControls();  // Update contextual controls
        }
    });
    m_mainToolBar->addAction(m_fogRectAction);

    // Create fog tool action group for mutual exclusivity
    QActionGroup* fogToolGroup = new QActionGroup(this);
    fogToolGroup->setExclusive(true);
    fogToolGroup->addAction(m_fogBrushAction);
    fogToolGroup->addAction(m_fogRectAction);

    // Reset Fog Button (in fog section, always visible)
    m_resetFogAction = new QAction(QIcon(":/icons/reset.svg"), "Reset Fog", this);
    m_resetFogAction->setToolTip("<b>Reset Fog</b><br>Clear all fog and start over<br><span style='color: #ff6b6b;'>⚠ Requires confirmation</span>");
    m_resetFogAction->setEnabled(false);  // Disabled until Fog Mode ON
    connect(m_resetFogAction, &QAction::triggered, this, &MainWindow::resetFogOfWar);
    m_mainToolBar->addAction(m_resetFogAction);

    m_mainToolBar->addSeparator();

    // SECTION 3: Zoom Controls (DM only - incremental zoom buttons)
    QAction* zoomOutAction = new QAction(QIcon(":/icons/zoom_out.svg"), "Zoom Out", this);
    zoomOutAction->setToolTip("<b>Zoom Out</b><br><i>Shortcut: -</i>");
    zoomOutAction->setShortcut(QKeySequence("-"));
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    m_mainToolBar->addAction(zoomOutAction);

    QAction* zoomInAction = new QAction(QIcon(":/icons/zoom_in.svg"), "Zoom In", this);
    zoomInAction->setToolTip("<b>Zoom In</b><br><i>Shortcut: +</i>");
    zoomInAction->setShortcut(QKeySequence("+"));
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    m_mainToolBar->addAction(zoomInAction);

    QAction* zoomFitAction = new QAction(QIcon(":/icons/zoom_fit.svg"), "Fit", this);
    zoomFitAction->setToolTip("<b>Fit to View</b><br>Fit entire map to window<br><i>Shortcut: 0</i>");
    zoomFitAction->setShortcut(QKeySequence("0"));
    connect(zoomFitAction, &QAction::triggered, this, &MainWindow::fitMapToView);
    m_mainToolBar->addAction(zoomFitAction);

    // Zoom level spinner (25-400%)
    QLabel* zoomLabel = new QLabel("Zoom:", this);
    zoomLabel->setStyleSheet("color: #E0E0E0; font-size: 12px; font-weight: 500; padding: 0 4px;");
    m_mainToolBar->addWidget(zoomLabel);

    m_zoomSpinner = new QSpinBox(this);
    m_zoomSpinner->setMinimum(10);   // Match MIN_ZOOM (0.1 * 100 = 10%)
    m_zoomSpinner->setMaximum(500);  // Match MAX_ZOOM (5.0 * 100 = 500%)
    m_zoomSpinner->setValue(100);    // 100% = actual size
    m_zoomSpinner->setSuffix("%");
    m_zoomSpinner->setSingleStep(5);  // Smaller increment for precision
    m_zoomSpinner->setFixedWidth(85);
    m_zoomSpinner->setToolTip("<b>Zoom Level</b><br>Adjust map zoom percentage<br>Range: 10-500%");
    m_zoomSpinner->setStyleSheet(R"(
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
    m_mainToolBar->addWidget(m_zoomSpinner);

    // Connect zoom spinner to actually change zoom
    // Use QPointer for safe lambda capture
    QPointer<QSpinBox> safeZoomSpinner = m_zoomSpinner;
    connect(m_zoomSpinner, QOverload<int>::of(&QSpinBox::valueChanged), [this, safeZoomSpinner](int value) {
        // Ignore programmatic updates (from zoom buttons)
        if (m_updatingZoomSpinner) {
            return;
        }

        if (m_mapDisplay && safeZoomSpinner) {
            qreal zoomFactor = value / 100.0;  // Convert percentage to factor
            m_mapDisplay->setZoomLevel(zoomFactor);
        }
    });

    m_mainToolBar->addSeparator();

    // SECTION 4: Grid Toggle (Simple checkbox)
    QAction* gridAction = new QAction(QIcon(":/icons/grid.svg"), "Grid", this);
    gridAction->setCheckable(true);
    gridAction->setChecked(m_gridEnabled);
    gridAction->setToolTip("<b>Grid Overlay</b><br>Show/hide grid lines<br><i>Shortcut: G</i>");
    gridAction->setShortcut(QKeySequence("G"));
    connect(gridAction, &QAction::triggered, this, &MainWindow::toggleGrid);
    m_mainToolBar->addAction(gridAction);
    m_toggleGridAction = gridAction;

    m_mainToolBar->addSeparator();

    // SECTION 5: Player Window (Most Important)
    QAction* playerWindowAction = new QAction(QIcon(":/icons/player_view.svg"), "Player View", this);
    playerWindowAction->setCheckable(true);
    playerWindowAction->setChecked(m_playerWindow != nullptr);
    playerWindowAction->setToolTip("<b>Player View</b><br>Open TV display window<br>Show map to players<br><i>Shortcut: P</i>");
    playerWindowAction->setShortcut(QKeySequence("P"));
    connect(playerWindowAction, &QAction::triggered, this, &MainWindow::togglePlayerWindow);
    m_mainToolBar->addAction(playerWindowAction);

    // Make player window button more prominent with animation
    if (auto* btn = qobject_cast<QToolButton*>(m_mainToolBar->widgetForAction(playerWindowAction))) {
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

        // Add pulse animation when not connected
        QTimer* pulseTimer = new QTimer(this);
        connect(pulseTimer, &QTimer::timeout, [btn, this]() {
            if (!m_playerWindow) {
                // Subtle pulse effect to draw attention
                static bool growing = true;
                static int pulseStep = 0;

                if (growing) {
                    pulseStep += 2;
                    if (pulseStep >= 10) growing = false;
                } else {
                    pulseStep -= 2;
                    if (pulseStep <= 0) growing = true;
                }

                QString glow = QString("0 0 %1px rgba(74, 144, 226, 0.6)").arg(10 + pulseStep);
                // Box-shadow not supported in Qt - removed to prevent warnings
            }
        });
        pulseTimer->start(100);
    }
    m_playerWindowToggleAction = playerWindowAction;


    // That's it! Just 3 tools + grid toggle + player window
    // No weather, no effects, no complex controls

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
    m_gridSizeSpinner->setMaximum(200);
    m_gridSizeSpinner->setValue(150);  // Default grid size (150px)
    m_gridSizeSpinner->setSuffix("px");
    m_gridSizeSpinner->setFixedWidth(85);  // Match brush spinner width
    m_gridSizeSpinner->setToolTip("<b>Grid Size</b><br>Adjust grid cell size<br>Range: 20-200px");
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

    // Get current tool type
    ToolType currentTool = m_toolManager ? m_toolManager->activeTool() : ToolType::Pointer;

    // === GLANCEABLE DESIGN (CLAUDE.md) ===
    // All sliders always visible - user can see what's possible at a glance
    // Disabled state makes it obvious when controls are inactive

    // === Brush Size Spinner ===
    // Always visible, enabled when Fog Mode ON
    if (m_fogBrushSizeSpinner) {
        m_fogBrushSizeSpinner->setEnabled(m_fogEnabled);
    }

    // === Grid Size Spinner ===
    // Always visible, enabled when Grid ON
    if (m_gridSizeSpinner) {
        m_gridSizeSpinner->setEnabled(m_gridEnabled);
    }

    // === Reset Fog Button ===
    // Always visible, enabled when Fog Mode is ON
    if (m_resetFogAction) {
        m_resetFogAction->setEnabled(m_fogEnabled);

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

    connect(m_toolboxWidget, &ToolboxWidget::fogBrushSizeChanged, [this](int size) {
        // TODO: Implement brush size handling in fog system
        // Store brush size in MainWindow for now
    });

    connect(m_toolboxWidget, &ToolboxWidget::gmOpacityChanged, [this](int opacity) {
        if (m_mapDisplay && m_mapDisplay->getFogOverlay()) {
            m_mapDisplay->getFogOverlay()->setGMOpacity(opacity / 100.0);
        }
    });

    m_toolboxWidget->updateGridStatus(m_gridEnabled);
    m_toolboxWidget->updateFogStatus(m_fogEnabled);
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
        std::cerr << "Creating TabsController on first map load..." << std::endl;

        m_tabsController = new TabsController(this);
        m_tabsController->attach(m_tabWidget, m_mapDisplay, MAX_TABS);

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
                        if (m_fogEnabled) {
                            m_fogAutosaveController->loadFromFile();
                        }
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
        m_playerWindow->setWindowTitle("LocalVTT - Player Display");
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
        std::cerr << "\n=== [MainWindow::togglePlayerWindow] OPENING PLAYER WINDOW - NEW ARCHITECTURE ===" << std::endl;
        m_playerWindow->show();

        QImage currentMap = m_mapDisplay->getCurrentMapImage();
        if (currentMap.isNull()) {
            std::cerr << "[MainWindow::togglePlayerWindow] WARNING: No map loaded yet" << std::endl;
        } else {
            std::cerr << "[MainWindow::togglePlayerWindow] Map loaded: " << currentMap.width() << "x" << currentMap.height() << std::endl;
        }
        std::cerr.flush();

        std::cerr << "[MainWindow::togglePlayerWindow] Calling forceRefresh() to copy map..." << std::endl;
        std::cerr.flush();
        m_playerWindow->forceRefresh();

        qreal currentZoom = m_mapDisplay->getZoomLevel();
        QPointF centerPoint = m_mapDisplay->mapToScene(m_mapDisplay->rect().center());
        std::cerr << "[MainWindow::togglePlayerWindow] Syncing zoom: " << currentZoom << std::endl;
        m_playerWindow->syncZoom(currentZoom, centerPoint);

        std::cerr << "=== [MainWindow::togglePlayerWindow] PLAYER WINDOW OPENED ===" << std::endl;
        std::cerr.flush();

        if (auto* toast = ToastNotification::instance(this)) {
            toast->showMessage("TV Display opened - drag window to TV", ToastNotification::Type::Success, 3000);
        }
    }

    // Update the toggle button state
    if (m_playerWindowToggleAction) {
        m_playerWindowToggleAction->setChecked(m_playerWindow && m_playerWindow->isVisible());
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
        // Sync MainWindow's state with GridController's state
        m_gridEnabled = m_gridController->isGridEnabled();

        // Show toast notification with correct NEW state
        if (auto* toast = ToastNotification::instance(this)) {
            QString message = m_gridEnabled ? "Grid shown" : "Grid hidden";
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
    m_fogEnabled = !m_fogEnabled;
    m_mapDisplay->setFogEnabled(m_fogEnabled);

    // Enable/disable fog tool buttons based on Fog Mode state
    if (m_fogBrushAction) {
        m_fogBrushAction->setEnabled(m_fogEnabled);
    }
    if (m_fogRectAction) {
        m_fogRectAction->setEnabled(m_fogEnabled);
    }

    // Auto-select Reveal Brush when fog is enabled
    if (m_fogEnabled && m_fogBrushAction) {
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
        action->setChecked(m_fogEnabled);
    }

    // Initialize fog autosave controller when fog is first enabled
    if (m_fogEnabled && !m_fogAutosaveController) {
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
    SettingsManager::instance().saveFogEnabled(m_fogEnabled);

    // Show toast notification
    if (auto* toast = ToastNotification::instance(this)) {
        QString message = m_fogEnabled ? "Fog of War enabled" : "Fog of War disabled";
        ToastNotification::Type type = m_fogEnabled ? ToastNotification::Type::Warning : ToastNotification::Type::Success;
        toast->showMessage(message, type, 2000);
    }

    // Update status indicators
    updateStatusIndicators();

    statusBar()->showMessage(m_fogEnabled ? "Fog of War enabled" : "Fog of War disabled", 2000);
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

            m_playerWindow->setWindowTitle("LocalVTT - Player Display");

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
    } catch (...) {
        DebugConsole::warning("Unknown exception during player window auto-open", "UI");
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
            QTimer::singleShot(150, [this, path]() {
                loadMapFile(path);
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
    m_gridStatusLabel->setProperty("active", m_gridEnabled);
    if (m_gridEnabled && m_mapDisplay && m_mapDisplay->getGridOverlay()) {
        GridOverlay* grid = m_mapDisplay->getGridOverlay();
        m_gridStatusLabel->setToolTip(QString("Grid: %1px = %2ft")
            .arg(grid->getGridSize())
            .arg(grid->getFeetPerSquare(), 0, 'f', 1));
    } else {
        m_gridStatusLabel->setToolTip("Grid overlay (Ctrl+G)");
    }

    // Minimalist fog indicator
    m_fogStatusLabel->setProperty("active", m_fogEnabled);
    m_fogStatusLabel->setToolTip(m_fogEnabled ? "Fog enabled" : "Fog disabled (Ctrl+F)");

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



void MainWindow::toggleFogHideMode()
{
    m_fogHideModeEnabled = !m_fogHideModeEnabled;

    if (m_mapDisplay) {
        m_mapDisplay->setFogHideModeEnabled(m_fogHideModeEnabled);

        // Enable fog mode if hide mode is turned on
        if (m_fogHideModeEnabled && !m_fogEnabled) {
            m_fogEnabled = true;
            m_mapDisplay->setFogEnabled(true);
        }

        // Turn off other modes when fog hide mode is enabled
        if (m_fogHideModeEnabled) {
            // Turn off rectangle mode if fog hide mode is enabled
            if (m_fogRectangleModeEnabled) {
                m_fogRectangleModeEnabled = false;
                m_mapDisplay->setFogRectangleModeEnabled(false);
            }
            statusBar()->showMessage("Fog hide mode enabled - left-click to hide, right-click to reveal", 3000);
        } else {
            statusBar()->showMessage("Fog hide mode disabled - left-click to reveal, right-click to hide", 3000);
        }

        updateStatusIndicators();

        // Sync with player window if open
        if (m_playerWindow) {
            m_playerWindow->forceRefresh();
        }
    }
}

void MainWindow::toggleFogRectangleMode()
{
    m_fogRectangleModeEnabled = !m_fogRectangleModeEnabled;

    // Update the menu action's checked state
    if (m_fogRectangleModeAction) {
        m_fogRectangleModeAction->setChecked(m_fogRectangleModeEnabled);
    }

    if (m_mapDisplay) {
        m_mapDisplay->setFogRectangleModeEnabled(m_fogRectangleModeEnabled);

        // Enable fog mode if rectangle mode is turned on
        if (m_fogRectangleModeEnabled && !m_fogEnabled) {
            m_fogEnabled = true;
            m_mapDisplay->setFogEnabled(true);
        }

        // Turn off other modes when fog rectangle mode is enabled
        if (m_fogRectangleModeEnabled) {
            // Turn off fog hide mode if fog rectangle mode is enabled
            if (m_fogHideModeEnabled) {
                m_fogHideModeEnabled = false;
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
    if (event->key() == Qt::Key_B) {
        if (event->modifiers() == Qt::ControlModifier) {
            // Ctrl+B = Intermission screen
            activateIntermission();
            event->accept();
            return;
        } else if (event->modifiers() == Qt::NoModifier) {
            // B = Blackout
            activateBlackout();
            event->accept();
            return;
        }
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
        case Qt::Key_0:
            // Fit to screen with "0" key
            fitToScreen();
            event->accept();
            return;
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

    // Create button group for future use if needed
    m_fogToolButtonGroup = new QButtonGroup(this);

    // Initialize action pointers to nullptr to prevent crashes
    m_revealCircleAction = nullptr;
    m_hideCircleAction = nullptr;
    m_revealRectangleAction = nullptr;
    m_hideRectangleAction = nullptr;
    m_revealFeatheredAction = nullptr;
    m_hideFeatheredAction = nullptr;
    m_drawPenAction = nullptr;
    m_drawEraserAction = nullptr;

    // Action triggers are connected via FogToolsController

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

    // Update legacy state variables for compatibility
    // Unified fog mode - modifier keys determine behavior
    m_fogHideModeEnabled = false;  // Will be determined by Alt key
    m_fogRectangleModeEnabled = false;  // Will be determined by Shift key

    // Update MapDisplay with new tool state
    if (m_mapDisplay) {
        m_mapDisplay->setFogHideModeEnabled(m_fogHideModeEnabled);
        m_mapDisplay->setFogRectangleModeEnabled(m_fogRectangleModeEnabled);

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
    
    // Check if using OpenGL display with HDR support
    auto* openglDisplay = m_mapDisplay->getOpenGLDisplay();
    if (openglDisplay && openglDisplay->isHDREnabled()) {
        openglDisplay->setExposure(exposure);
    }

    // Update label
    if (m_exposureLabel) {
        m_exposureLabel->setText(QString("Exposure: %1").arg(exposure, 0, 'f', 2));
    }

    // Sync with player window
    if (m_playerWindow && m_playerWindow->getMapDisplay()) {
        auto* playerOpenGL = m_playerWindow->getMapDisplay()->getOpenGLDisplay();
        if (playerOpenGL && playerOpenGL->isHDREnabled()) {
            playerOpenGL->setExposure(exposure);
        }
    }

    statusBar()->showMessage(QString("HDR exposure set to %1").arg(exposure, 0, 'f', 2), 2000);
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
        case FogToolMode::DrawPen:
            return "Draw (Pen)";
        case FogToolMode::DrawEraser:
            return "Draw (Eraser)";
        default:
            return "Unknown";
    }
}

QString MainWindow::getFogToolModeInstructions(FogToolMode mode) const
{
    switch (mode) {
        case FogToolMode::UnifiedFog:
            return "Click=Reveal, Alt+Click=Hide, Shift=Rectangle, Double-click=Clear visible, [/]=Size";
        case FogToolMode::DrawPen:
            return "Click and drag to draw lines";
        case FogToolMode::DrawEraser:
            return "Click to erase drawings";
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
    // Point light feature removed - no longer supported
    Q_UNUSED(lightId);
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
        m_toggleGridAction->setChecked(m_gridEnabled);
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

void MainWindow::showKeyboardShortcuts()
{
    if (!m_actionRegistry) {
        QMessageBox::warning(this, "Error", "ActionRegistry not available.");
        return;
    }
    
    QStringList descriptions = m_actionRegistry->getAllShortcutDescriptions();

    QString shortcutsText = "<h3>Keyboard Shortcuts</h3><pre style='font-family: monospace; font-size: 11pt;'>";
    shortcutsText += descriptions.join("\n");
    shortcutsText += "</pre>";

    QMessageBox::information(this, "Keyboard Shortcuts", shortcutsText);
}

void MainWindow::showQuickStartGuide()
{
    QString guideText =
        "<h3>Quick Start Guide</h3>"
        "<p><b>1. Connect TV as second display</b><br>"
        "Set up your TV or second monitor as an extended display.</p>"
        "<p><b>2. Launch LocalVTT</b><br>"
        "The application will open with the main control window.</p>"
        "<p><b>3. Drag map onto main window</b><br>"
        "Drop any image file or VTT file onto the main window to load it.</p>"
        "<p><b>4. Player window appears automatically</b><br>"
        "The player view opens automatically on your TV/second display.</p>"
        "<p><b>5. Use fog tools to hide/reveal areas</b><br>"
        "Use R/H keys or Shift+R/H for different fog tools to control what players see.</p>"
        "<p><i>Just maps on a TV - that's the entire scope!</i></p>";

    QMessageBox::information(this, "Quick Start Guide", guideText);
}

void MainWindow::showAboutDialog()
{
    QString aboutText =
        "<h3>LocalVTT v1.0.0</h3>"
        "<p><b>Digital battle mat for in-person tabletop gaming</b></p>"
        "<p><i>Just maps on a TV</i></p>"
        "<p>Copyright © 2024 LocalVTT<br>"
        "Licensed under the MIT License</p>"
        "<p>A simple, focused virtual tabletop designed specifically for<br>"
        "displaying maps on a TV during in-person tabletop gaming sessions.</p>";

    QMessageBox::about(this, "About LocalVTT", aboutText);
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
