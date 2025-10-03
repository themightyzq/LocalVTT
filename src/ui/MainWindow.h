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
class QPropertyAnimation;
class QAction;
class QLabel;
class QWidget;
class QTimer;
class QTabWidget;
class QSlider;
class QSpinBox;
class QButtonGroup;
class RecentFilesController;
class FogAutosaveController;
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

    // Measurement tool controls

    // Unified Fog Tool Mode System
    void setFogToolMode(FogToolMode mode);
    FogToolMode getFogToolMode() const { return m_fogToolMode; }


    // Legacy compatibility (will be removed)
    void toggleFogHideMode();
    void toggleFogRectangleMode();

    // Player view mode controls
    void togglePlayerViewMode();

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

    // Help menu controls
    void showKeyboardShortcuts();
    void showQuickStartGuide();
    void showAboutDialog();

    // Tab controls
    void onTabChanged(int index);
    void onTabCloseRequested(int index);

protected:
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

    // Tab system
    QTabWidget* m_tabWidget;
    static const int MAX_TABS = 10;

    QMenu* m_fileMenu;
    QMenu* m_recentFilesMenu;
    QMenu* m_viewMenu;
    QMenu* m_windowMenu;
    QMenu* m_helpMenu;
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

    bool m_gridEnabled;
    bool m_fogEnabled;
    bool m_isDragging;
    bool m_updatingZoomSpinner;  // Flag to prevent feedback loops
    // Unified fog tool state
    FogToolMode m_fogToolMode;

    // Legacy state variables (for compatibility)
    bool m_fogHideModeEnabled;
    bool m_fogRectangleModeEnabled;
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
    ToolStatusWidget* m_toolStatusWidget;

    // Zoom controls
    QAction* m_fitToScreenAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QLabel* m_zoomPercentageLabel;

    // Grid controls
    QSpinBox* m_gridSizeSpinner;
    QLabel* m_gridSizeLabel;

    // Zoom controls
    QSpinBox* m_zoomSpinner;

    // Fog brush controls
    QSlider* m_fogBrushSlider;
    QLabel* m_fogBrushLabel;

    // GM opacity controls
    QSlider* m_gmOpacitySlider;
    QLabel* m_gmOpacityLabel;

    // Fog tool mode controls
    QButtonGroup* m_fogToolButtonGroup;
    QAction* m_revealCircleAction;
    QAction* m_hideCircleAction;
    QAction* m_revealRectangleAction;
    QAction* m_hideRectangleAction;
    QAction* m_revealFeatheredAction;
    QAction* m_hideFeatheredAction;
    QAction* m_drawPenAction;
    QAction* m_drawEraserAction;
    QLabel* m_fogToolModeLabel;
    // Fog tools controller
    FogToolsController* m_fogToolsController;

    // Undo/Redo controls
    QAction* m_undoAction;
    QAction* m_redoAction;

    // Grid controls
    QAction* m_toggleGridAction;

    // Player Window toggle action
    QAction* m_playerWindowToggleAction;

    // Fog tool actions for state management
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

    // Tab management
    bool hasMaxTabs() const { return false; }

    // Fog tool mode helpers
    void setupFogToolModeSystem();
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
