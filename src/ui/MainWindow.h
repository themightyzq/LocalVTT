#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QUuid>
#include <memory>
#include "utils/FogToolMode.h"
#include "utils/ActionRegistry.h"

class MapDisplay;
class PlayerWindow;
class MapSession;
class ToolboxWidget;
class QMenu;
class QToolBar;
class QProgressDialog;
class QWidgetAction;
class LoadingOverlay;
class KeyboardShortcutsOverlay;
class QPropertyAnimation;
class QAction;
class QLabel;
class QWidget;
class QTimer;
class QTabBar;
class QSlider;
class QSpinBox;
class QButtonGroup;
class RecentFilesController;
class FogAutosaveController;
class QScreen;
class TabsController;
class FogToolsController;
class DebugConsoleWidget;
class MenuManager;
class FileOperationsManager;
class ToolManager;
class GridController;
class LightingController;
class ViewZoomController;
class PlayerWindowController;
class ToolStatusWidget;
class AtmosphereController;
class AtmosphereToolboxWidget;
class MapBrowserWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Public API for loading maps from command line
    void loadMapFromCommandLine(const QString& path) { loadMapFile(path); }

public slots:
    void loadMap();
    void togglePlayerWindow();
    void toggleGrid();
    void toggleGridType();
    void showGridInfo();
    void setStandardGrid();
    void openPreferences();
    void toggleFogOfWar();
    void clearFogOfWar();
    void resetFogOfWar();
    void openRecentFile();
    void clearRecentFiles();

    // Privacy Shield controls
    void activateBlackout();
    void activateIntermission();
    void deactivatePrivacyMode();
    void updatePrivacyStatusIndicator(bool privacyActive, const QString& modeText = QString());

    // Zoom controls
    void fitToScreen();
    void fitMapToView() { fitToScreen(); }  // Alias for toolbar button
    void zoomIn();
    void zoomOut();
    void centerOnMap();  // Re-center view on map without changing zoom
    void toggleFogLock();   // Toggle fog editing lock
    void toggleGridLock();  // Toggle grid editing lock
    void rotateMap();       // Rotate map 90° clockwise
    void rotatePlayerView();  // Rotate only the player view 90° clockwise
    void toggleRotationSync();  // Toggle rotation sync to player window
    void syncRotationToPlayer();  // Manually sync DM rotation to player
    bool isRotationSyncEnabled() const { return m_syncRotationToPlayer; }

    // Measurement tool controls

    // Unified Fog Tool Mode System
    void setFogToolMode(FogToolMode mode);
    FogToolMode getFogToolMode() const { return m_fogToolMode; }


    // Fog mode toggle methods (used with modifier keys: Alt=hide, Shift=rectangle)
    void toggleFogHideMode();
    void toggleFogRectangleMode();

    // Player view mode controls
    void togglePlayerViewMode();

    // Player View sync controls
    void syncViewToPlayers();   // Push DM's current view to player window
    void resetPlayerAutoFit();  // Reset player window to auto-fit mode

    // Undo/Redo controls
    void undoFogChange();
    void redoFogChange();

    // Lighting system controls
    void toggleLighting();
    void setTimeOfDay(int timeOfDay);
    void onLightingIntensityChanged(int value);

    // Point light controls
    void togglePointLightPlacement();
    void onAmbientLightChanged(int value);
    void clearAllPointLights();
    void showPointLightProperties(const QUuid& lightId);

    // HDR exposure controls
    void onExposureChanged(int value);


    // Debug Console controls
    void toggleDebugConsole();

    // Map Browser controls
    void toggleMapBrowser();

    // Atmosphere Toolbox controls
    void toggleAtmosphereToolbox();

    // Help menu controls
    void showKeyboardShortcuts();
    void showQuickStartGuide();
    void showAboutDialog();

    // Tab controls
    void onTabChanged(int index);
    void onTabCloseRequested(int index);

protected:
    // Window close event - ensures PlayerWindow closes with MainWindow
    void closeEvent(QCloseEvent *event) override;

    // Enhanced drag and drop support
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    // Keyboard event handling for zoom shortcuts
    void keyPressEvent(QKeyEvent *event) override;
    
    // Geometry persistence
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // Focus management - ensure window has focus when shown
    void showEvent(QShowEvent *event) override;

private slots:
    void handleLoadProgress(qint64 bytesRead, qint64 totalBytes);
    void animateDropFeedback(bool entering);
    void updateZoomStatus();
    void syncZoomWithPlayer(qreal zoomLevel);
    void saveFogState();
    void onFogChanged();
    void handleDisplayFogChanged();
    void quickSaveFogState();
    void quickRestoreFogState();
    void onGridSizeChanged(int value);
    void updateGridSizeSlider();
    void onFogBrushSizeChanged(int value);
    void setSmallBrush();
    void setMediumBrush();
    void setLargeBrush();
    void onGMOpacityChanged(int value);
    void updateUndoRedoButtons();
    void updateLightingControls();

private:
    void setupMinimalUI();  // Create only essential components
    void setupDeferredComponents();  // Create heavy components after startup
    void createMenus();
    void setupActions();
    QAction* getOrCreateAction(const QString& actionId);
    void setupToolbox();
    void createToolbar();
    void createZoomToolbar();
    void loadMapFile(const QString& path);
    void updateRecentFilesMenu();
    void addToRecentFiles(const QString& filePath);
    void autoOpenPlayerWindow();
    void positionPlayerWindow();
    void ensurePlayerWindowConnections();
    void updateSendToDisplayMenu();  // Multi-monitor support
    void sendPlayerWindowToScreen(QScreen* screen);
    void showLoadProgress(const QString& fileName, qint64 fileSize);
    void hideLoadProgress();
    void setupStatusBar();
    void updateStatusIndicators();
    QString getFogFilePath(const QString& mapPath) const;
    void loadFogState(const QString& mapPath);
    void scheduleFogSave();
    void showAutosaveNotification(const QString& message);
    void switchToTab(int index);
    void closeTab(int index);

    MapDisplay* m_mapDisplay;
    PlayerWindow* m_playerWindow;
    ToolboxWidget* m_toolboxWidget;

    // Core systems
    ActionRegistry* m_actionRegistry;

    // Manager classes for better organization
    MenuManager* m_menuManager;
    FileOperationsManager* m_fileOperationsManager;
    ToolManager* m_toolManager;

    // New controllers for refactored functionality
    GridController* m_gridController;
    LightingController* m_lightingController;
    ViewZoomController* m_viewZoomController;
    PlayerWindowController* m_playerWindowController;
    AtmosphereController* m_atmosphereController;

    // Tab system
    QTabBar* m_tabBar;
    static const int MAX_TABS = 10;

    QMenu* m_fileMenu;
    QMenu* m_recentFilesMenu;
    QMenu* m_viewMenu;
    QMenu* m_windowMenu;
    QMenu* m_helpMenu;
    QMenu* m_sendToDisplayMenu;  // Multi-monitor support
    QToolBar* m_mainToolBar;
    QToolBar* m_zoomToolBar;

    // Recent files management
    QList<QAction*> m_recentFileActions;
    QAction* m_clearRecentAction;
    RecentFilesController* m_recentFilesController;
    TabsController* m_tabsController;
    static const int MaxRecentFiles = 10;

    // Progress loading
    QProgressDialog* m_progressDialog;
    LoadingOverlay* m_loadingOverlay;
    QElapsedTimer m_loadTimer;

    // Drag and drop visual feedback
    QWidget* m_dropOverlay;
    QPropertyAnimation* m_dropAnimation;

    bool m_fogLocked;   // When true, fog tools are disabled
    bool m_gridLocked;  // When true, grid controls are disabled
    int m_mapRotation;  // Current map rotation (0, 90, 180, 270)
    int m_playerRotation;  // Player window rotation (independent when sync disabled)
    bool m_syncRotationToPlayer;  // When true, rotation syncs to player window
    bool m_isDragging;
    bool m_updatingZoomSpinner;  // Flag to prevent feedback loops
    // Unified fog tool state
    FogToolMode m_fogToolMode;

    bool m_playerViewModeEnabled;
    static const int AUTOSAVE_DELAY_MS = 500;

    // Status bar indicators
    QWidget* m_statusContainer;
    QLabel* m_gridStatusLabel;
    QLabel* m_fogStatusLabel;
    QLabel* m_playerViewStatusLabel;
    QLabel* m_zoomStatusLabel;
    QLabel* m_privacyStatusLabel;
    QLabel* m_playerSyncBadge;
    QLabel* m_rotationStatusLabel;  // Shows DM/Player rotation state
    ToolStatusWidget* m_toolStatusWidget;

    // Zoom controls
    QAction* m_fitToScreenAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QLabel* m_zoomPercentageLabel;

    // Grid controls
    QSpinBox* m_gridSizeSpinner;
    QLabel* m_gridSizeLabel;
    QAction* m_lockGridAction;  // Toggle grid lock

    // Zoom controls
    QSpinBox* m_zoomSpinner;

    // Fog brush controls
    QAction* m_lockFogAction;   // Toggle fog lock
    QSlider* m_fogBrushSlider;
    QLabel* m_fogBrushLabel;

    // GM opacity controls
    QSlider* m_gmOpacitySlider;
    QLabel* m_gmOpacityLabel;

    // Fog tools controller
    FogToolsController* m_fogToolsController;

    // Undo/Redo controls
    QAction* m_undoAction;
    QAction* m_redoAction;

    // Grid controls
    QAction* m_toggleGridAction;

    // Player Window toggle action
    QAction* m_playerWindowToggleAction;

    // Player View sync actions
    QAction* m_syncViewToPlayersAction;
    QAction* m_resetPlayerAutoFitAction;

    // Rotation sync actions
    QAction* m_syncRotationToggleAction;  // Toggle auto-sync rotation
    QAction* m_syncRotationNowAction;     // Manual sync rotation to player
    QAction* m_rotatePlayerViewAction;    // Rotate only player view

    // Fog tool actions for state management
    QAction* m_fogHideToggleAction;
    QAction* m_fogBrushAction;
    QAction* m_fogRectAction;
    QAction* m_fogRectangleModeAction;
    QSpinBox* m_fogBrushSizeSpinner;

    // Contextual toolbar controls
    QAction* m_resetFogAction;
    QTimer* m_brushSizeDebounceTimer;
    QTimer* m_gridSizeDebounceTimer;
    QDockWidget* m_controlStripDock;  // Secondary toolbar for sliders

    // Lighting controls
    QAction* m_lightingAction;
    QButtonGroup* m_timeOfDayButtonGroup;
    QAction* m_dawnAction;
    QAction* m_dayAction;
    QAction* m_duskAction;
    QAction* m_nightAction;
    QSlider* m_lightingIntensitySlider;
    QLabel* m_lightingIntensityLabel;
    QLabel* m_timeOfDayLabel;

    // Point light controls
    QAction* m_pointLightPlacementAction;
    QSlider* m_ambientLightSlider;
    QLabel* m_ambientLightLabel;
    QAction* m_clearPointLightsAction;

    // HDR exposure controls
    QSlider* m_exposureSlider;
    QLabel* m_exposureLabel;

    // Autosave functionality
    FogAutosaveController* m_fogAutosaveController;
    QString m_currentMapPath;


    // Debug Console
    DebugConsoleWidget* m_debugConsoleWidget;
    QAction* m_debugConsoleAction;

    // Keyboard shortcuts overlay
    KeyboardShortcutsOverlay* m_shortcutsOverlay;

    // Map Browser
    MapBrowserWidget* m_mapBrowserWidget;
    QAction* m_mapBrowserAction;

    // Atmosphere Toolbox
    AtmosphereToolboxWidget* m_atmosphereToolbox;
    QAction* m_atmosphereToolboxAction;

    // Fog tool mode helpers
    void setupFogToolModeSystem();
    void updateFogModeIndicator();
    void updateFogToolModeUI();
    void updateFogToolModeStatus();
    QString getFogToolModeText(FogToolMode mode) const;
    QString getFogToolModeInstructions(FogToolMode mode) const;

    // Weather and effects helpers
    void updateAllMenuStates();

    // Contextual toolbar helpers
    void updateContextualToolbarControls();
    void createContextualControls();
};

#endif // MAINWINDOW_H
