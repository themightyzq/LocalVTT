#include "controllers/MenuManager.h"
#include "ui/MainWindow.h"
#include "controllers/RecentFilesController.h"
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QKeySequence>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QWidgetAction>

MenuManager::MenuManager(MainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_recentFilesController(nullptr)
    , m_fileMenu(nullptr)
    , m_viewMenu(nullptr)
    , m_toolsMenu(nullptr)
    , m_windowMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_recentFilesMenu(nullptr)
    , m_loadMapAction(nullptr)
    , m_toggleGridAction(nullptr)
    , m_toggleFogAction(nullptr)
    , m_clearFogAction(nullptr)
    , m_resetFogAction(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_fitToScreenAction(nullptr)
    , m_zoomInAction(nullptr)
    , m_zoomOutAction(nullptr)
    , m_togglePlayerWindowAction(nullptr)
    , m_openPreferencesAction(nullptr)
    , m_clearRecentAction(nullptr)
    , m_unifiedFogAction(nullptr)
    , m_drawPenAction(nullptr)
    , m_drawEraserAction(nullptr)
{
}

void MenuManager::createMenus()
{
    createFileMenu();
    createViewMenu();
    createToolsMenu();
    createWindowMenu();
    createHelpMenu();
}

void MenuManager::createFileMenu()
{
    m_fileMenu = m_mainWindow->menuBar()->addMenu("&File");

    m_loadMapAction = new QAction("&Load Map...", m_mainWindow);
    m_loadMapAction->setShortcut(QKeySequence::Open);
    m_loadMapAction->setStatusTip("Load a map image or VTT file");
    connect(m_loadMapAction, &QAction::triggered, this, &MenuManager::loadMapRequested);
    m_fileMenu->addAction(m_loadMapAction);

    m_fileMenu->addSeparator();

    m_recentFilesMenu = m_fileMenu->addMenu("&Recent Files");
    m_recentFilesController = new RecentFilesController(m_mainWindow);

    for (int i = 0; i < MaxRecentFiles; ++i) {
        auto* action = new QAction(m_mainWindow);
        action->setVisible(false);
        connect(action, &QAction::triggered, [this, action]() {
            auto path = action->data().toString();
            if (!path.isEmpty()) {
                emit loadMapRequested();
            }
        });
        m_recentFileActions.append(action);
        m_recentFilesMenu->addAction(action);
    }

    m_recentFilesMenu->addSeparator();

    m_clearRecentAction = new QAction("&Clear Recent Files", m_mainWindow);
    m_recentFilesMenu->addAction(m_clearRecentAction);

    m_recentFilesController->attach(m_recentFilesMenu, m_recentFileActions, m_clearRecentAction, MaxRecentFiles);
    connect(m_recentFilesController, &RecentFilesController::openFileRequested,
            this, &MenuManager::loadMapRequested);

    m_fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", m_mainWindow);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, m_mainWindow, &MainWindow::close);
    m_fileMenu->addAction(exitAction);
}

void MenuManager::createViewMenu()
{
    m_viewMenu = m_mainWindow->menuBar()->addMenu("&View");

    m_toggleGridAction = new QAction("Toggle &Grid", m_mainWindow);
    m_toggleGridAction->setShortcut(QKeySequence("Ctrl+G"));
    m_toggleGridAction->setCheckable(true);
    connect(m_toggleGridAction, &QAction::triggered, this, &MenuManager::toggleGridRequested);
    m_viewMenu->addAction(m_toggleGridAction);

    createGridSubmenu(m_viewMenu);

    m_toggleFogAction = new QAction("Toggle &Fog of War", m_mainWindow);
    m_toggleFogAction->setShortcut(QKeySequence("Ctrl+F"));
    m_toggleFogAction->setCheckable(true);
    connect(m_toggleFogAction, &QAction::triggered, this, &MenuManager::toggleFogRequested);
    m_viewMenu->addAction(m_toggleFogAction);

    m_clearFogAction = new QAction("&Clear Fog of War", m_mainWindow);
    connect(m_clearFogAction, &QAction::triggered, this, &MenuManager::clearFogRequested);
    m_viewMenu->addAction(m_clearFogAction);

    m_resetFogAction = new QAction("&Reset Fog of War", m_mainWindow);
    m_resetFogAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(m_resetFogAction, &QAction::triggered, this, &MenuManager::resetFogRequested);
    m_viewMenu->addAction(m_resetFogAction);

    createFogSubmenu(m_viewMenu);

    m_viewMenu->addSeparator();

    m_fitToScreenAction = new QAction("&Fit to Screen", m_mainWindow);
    m_fitToScreenAction->setShortcut(QKeySequence("0"));
    connect(m_fitToScreenAction, &QAction::triggered, this, &MenuManager::fitToScreenRequested);
    m_viewMenu->addAction(m_fitToScreenAction);

    m_zoomInAction = new QAction("Zoom &In", m_mainWindow);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(m_zoomInAction, &QAction::triggered, this, &MenuManager::zoomInRequested);
    m_viewMenu->addAction(m_zoomInAction);

    m_zoomOutAction = new QAction("Zoom &Out", m_mainWindow);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(m_zoomOutAction, &QAction::triggered, this, &MenuManager::zoomOutRequested);
    m_viewMenu->addAction(m_zoomOutAction);

    m_viewMenu->addSeparator();
}

void MenuManager::createToolsMenu()
{
    m_toolsMenu = m_mainWindow->menuBar()->addMenu("&Tools");

    QAction* measurementAction = new QAction("&Measurement Tool", m_mainWindow);
    measurementAction->setCheckable(true);
    measurementAction->setShortcut(QKeySequence("M"));
    m_toolsMenu->addAction(measurementAction);

    m_toolsMenu->addSeparator();

    m_undoAction = new QAction("&Undo Fog Change", m_mainWindow);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);
    m_toolsMenu->addAction(m_undoAction);

    m_redoAction = new QAction("&Redo Fog Change", m_mainWindow);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(false);
    m_toolsMenu->addAction(m_redoAction);

    m_toolsMenu->addSeparator();

    createLightingSubmenu(m_toolsMenu);
}

void MenuManager::createWindowMenu()
{
    m_windowMenu = m_mainWindow->menuBar()->addMenu("&Window");

    m_togglePlayerWindowAction = new QAction("Toggle &Player Window", m_mainWindow);
    m_togglePlayerWindowAction->setShortcut(QKeySequence("Space"));
    connect(m_togglePlayerWindowAction, &QAction::triggered, this, &MenuManager::togglePlayerWindowRequested);
    m_windowMenu->addAction(m_togglePlayerWindowAction);

    m_windowMenu->addSeparator();

    QAction* debugConsoleAction = new QAction("&Debug Console", m_mainWindow);
    debugConsoleAction->setShortcut(QKeySequence("F12"));
    m_windowMenu->addAction(debugConsoleAction);

    m_windowMenu->addSeparator();

    m_openPreferencesAction = new QAction("&Preferences...", m_mainWindow);
    m_openPreferencesAction->setShortcut(QKeySequence::Preferences);
    connect(m_openPreferencesAction, &QAction::triggered, this, &MenuManager::openPreferencesRequested);
    m_windowMenu->addAction(m_openPreferencesAction);
}

void MenuManager::createHelpMenu()
{
    m_helpMenu = m_mainWindow->menuBar()->addMenu("&Help");

    QAction* keyboardShortcutsAction = new QAction("&Keyboard Shortcuts", m_mainWindow);
    m_helpMenu->addAction(keyboardShortcutsAction);

    QAction* quickStartAction = new QAction("&Quick Start Guide", m_mainWindow);
    m_helpMenu->addAction(quickStartAction);

    m_helpMenu->addSeparator();

    QAction* aboutAction = new QAction("&About LocalVTT", m_mainWindow);
    connect(aboutAction, &QAction::triggered, this, &MenuManager::showAboutRequested);
    m_helpMenu->addAction(aboutAction);
}

void MenuManager::createFogSubmenu(QMenu* parentMenu)
{
    QMenu* fogToolsMenu = parentMenu->addMenu("Fog &Tools");

    // Single unified fog tool action
    m_unifiedFogAction = new QAction("&Unified Fog Tool", m_mainWindow);
    m_unifiedFogAction->setShortcut(QKeySequence("F"));
    m_unifiedFogAction->setCheckable(true);
    m_unifiedFogAction->setStatusTip("Use Shift for hide, Alt for rectangle selection");
    connect(m_unifiedFogAction, &QAction::triggered, this, [this]() {
        emit fogToolModeChanged(FogToolMode::UnifiedFog);
    });
    fogToolsMenu->addAction(m_unifiedFogAction);

    fogToolsMenu->addSeparator();

    // Drawing tools (preserved for drawing system)
    m_drawPenAction = new QAction("&Draw Pen", m_mainWindow);
    m_drawPenAction->setShortcut(QKeySequence("P"));
    m_drawPenAction->setCheckable(true);
    connect(m_drawPenAction, &QAction::triggered, this, [this]() {
        emit fogToolModeChanged(FogToolMode::DrawPen);
    });
    fogToolsMenu->addAction(m_drawPenAction);

    m_drawEraserAction = new QAction("&Draw Eraser", m_mainWindow);
    m_drawEraserAction->setShortcut(QKeySequence("E"));
    m_drawEraserAction->setCheckable(true);
    connect(m_drawEraserAction, &QAction::triggered, this, [this]() {
        emit fogToolModeChanged(FogToolMode::DrawEraser);
    });
    fogToolsMenu->addAction(m_drawEraserAction);
}

void MenuManager::createGridSubmenu(QMenu* parentMenu)
{
    QMenu* gridConfigMenu = parentMenu->addMenu("Grid &Configuration");

    QAction* gridInfoAction = new QAction("Show Grid &Info", m_mainWindow);
    gridInfoAction->setShortcut(QKeySequence("Ctrl+I"));
    gridConfigMenu->addAction(gridInfoAction);

    gridConfigMenu->addSeparator();

    QAction* setStandardGridAction = new QAction("Reset to D&&D &Standard", m_mainWindow);
    gridConfigMenu->addAction(setStandardGridAction);
}

void MenuManager::createLightingSubmenu(QMenu* parentMenu)
{
    QMenu* lightingMenu = parentMenu->addMenu("&Lighting");

    QAction* enableLightingAction = new QAction("&Enable Lighting", m_mainWindow);
    enableLightingAction->setCheckable(true);
    lightingMenu->addAction(enableLightingAction);

    lightingMenu->addSeparator();

    QMenu* timeOfDayMenu = lightingMenu->addMenu("&Time of Day");

    QAction* dawnAction = new QAction("&Dawn", m_mainWindow);
    dawnAction->setCheckable(true);
    timeOfDayMenu->addAction(dawnAction);

    QAction* dayAction = new QAction("D&ay", m_mainWindow);
    dayAction->setCheckable(true);
    dayAction->setChecked(true);
    timeOfDayMenu->addAction(dayAction);

    QAction* duskAction = new QAction("D&usk", m_mainWindow);
    duskAction->setCheckable(true);
    timeOfDayMenu->addAction(duskAction);

    QAction* nightAction = new QAction("&Night", m_mainWindow);
    nightAction->setCheckable(true);
    timeOfDayMenu->addAction(nightAction);

    QButtonGroup* timeOfDayGroup = new QButtonGroup(m_mainWindow);
    timeOfDayGroup->setExclusive(true);
}


void MenuManager::updateAllMenuStates()
{
    if (m_toggleGridAction) {
        bool gridEnabled = false;
        m_toggleGridAction->setChecked(gridEnabled);
    }

    if (m_toggleFogAction) {
        bool fogEnabled = false;
        m_toggleFogAction->setChecked(fogEnabled);
    }
}

void MenuManager::updateRecentFilesMenu()
{
    if (m_recentFilesController) {
        m_recentFilesController->updateMenu();
    }
}

void MenuManager::updateFogToolMenuState(FogToolMode mode)
{
    // Clear all fog tool action checked states first
    if (m_unifiedFogAction) m_unifiedFogAction->setChecked(false);
    if (m_drawPenAction) m_drawPenAction->setChecked(false);
    if (m_drawEraserAction) m_drawEraserAction->setChecked(false);

    // Set the appropriate action as checked based on the current mode
    switch (mode) {
        case FogToolMode::UnifiedFog:
            if (m_unifiedFogAction) m_unifiedFogAction->setChecked(true);
            break;
        case FogToolMode::DrawPen:
            if (m_drawPenAction) m_drawPenAction->setChecked(true);
            break;
        case FogToolMode::DrawEraser:
            if (m_drawEraserAction) m_drawEraserAction->setChecked(true);
            break;
        default:
            break;
    }
}