#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include <QObject>
#include <QList>
#include "utils/FogToolMode.h"

class QMainWindow;
class QMenu;
class QAction;
class QToolBar;
class MainWindow;
class RecentFilesController;

class MenuManager : public QObject
{
    Q_OBJECT

public:
    explicit MenuManager(MainWindow* mainWindow, QObject* parent = nullptr);
    ~MenuManager() = default;

    void createMenus();
    void updateAllMenuStates();
    void updateRecentFilesMenu();
    void updateFogToolMenuState(FogToolMode mode);

    QMenu* fileMenu() const { return m_fileMenu; }
    QMenu* viewMenu() const { return m_viewMenu; }
    QMenu* toolsMenu() const { return m_toolsMenu; }
    QMenu* windowMenu() const { return m_windowMenu; }
    QMenu* helpMenu() const { return m_helpMenu; }

    QAction* loadMapAction() const { return m_loadMapAction; }
    QAction* toggleGridAction() const { return m_toggleGridAction; }
    QAction* toggleFogAction() const { return m_toggleFogAction; }
    QAction* undoAction() const { return m_undoAction; }
    QAction* redoAction() const { return m_redoAction; }

signals:
    void loadMapRequested();
    void toggleGridRequested();
    void toggleFogRequested();
    void clearFogRequested();
    void resetFogRequested();
    void fitToScreenRequested();
    void zoomInRequested();
    void zoomOutRequested();
    void openPreferencesRequested();
    void togglePlayerWindowRequested();
    void showAboutRequested();
    void fogToolModeChanged(FogToolMode mode);

private:
    void createFileMenu();
    void createViewMenu();
    void createToolsMenu();
    void createWindowMenu();
    void createHelpMenu();
    void createFogSubmenu(QMenu* parentMenu);
    void createGridSubmenu(QMenu* parentMenu);
    void createLightingSubmenu(QMenu* parentMenu);

    MainWindow* m_mainWindow;
    RecentFilesController* m_recentFilesController;

    QMenu* m_fileMenu;
    QMenu* m_viewMenu;
    QMenu* m_toolsMenu;
    QMenu* m_windowMenu;
    QMenu* m_helpMenu;
    QMenu* m_recentFilesMenu;

    QAction* m_loadMapAction;
    QAction* m_toggleGridAction;
    QAction* m_toggleFogAction;
    QAction* m_clearFogAction;
    QAction* m_resetFogAction;
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_fitToScreenAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_togglePlayerWindowAction;
    QAction* m_openPreferencesAction;
    QAction* m_clearRecentAction;

    // Fog tool actions for synchronization
    QAction* m_unifiedFogAction;
    QAction* m_drawPenAction;
    QAction* m_drawEraserAction;

    static const int MaxRecentFiles = 10;
    QList<QAction*> m_recentFileActions;
};

#endif // MENUMANAGER_H