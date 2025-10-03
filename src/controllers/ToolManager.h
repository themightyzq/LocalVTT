#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include <QObject>
#include "utils/FogToolMode.h"
#include "utils/ToolType.h"

class MainWindow;
class MapDisplay;
class QAction;
class QSlider;
class QLabel;
class QButtonGroup;

class ToolManager : public QObject
{
    Q_OBJECT

public:
    explicit ToolManager(MainWindow* mainWindow, MapDisplay* mapDisplay, QObject* parent = nullptr);
    ~ToolManager() = default;

    void setupFogToolModeSystem();
    void setFogToolMode(FogToolMode mode);
    FogToolMode getFogToolMode() const { return m_fogToolMode; }

    void setActiveTool(ToolType tool);
    ToolType activeTool() const { return m_activeToolType; }
    QString getActiveToolName() const;
    bool handleEscapeKey();

    void toggleFogOfWar();
    void clearFogOfWar();
    void resetFogOfWar();

    void toggleGrid();
    void toggleGridType();
    void showGridInfo();
    void setStandardGrid();


    void togglePlayerViewMode();

    void undoFogChange();
    void redoFogChange();

    void setSmallBrush();
    void setMediumBrush();
    void setLargeBrush();

    void onGridSizeChanged(int value);
    void onFogBrushSizeChanged(int value);
    void onGMOpacityChanged(int value);

    void updateGridSizeSlider();
    void updateUndoRedoButtons();
    void updateFogToolModeUI();
    void updateFogToolModeStatus();

    bool isGridEnabled() const { return m_gridEnabled; }
    bool isFogEnabled() const { return m_fogEnabled; }
    bool isPlayerViewModeEnabled() const { return m_playerViewModeEnabled; }

    void attachGridSlider(QSlider* slider, QLabel* label);
    void attachFogBrushSlider(QSlider* slider, QLabel* label);
    void attachFogToolActions(QAction* unifiedFog, QAction* drawPen, QAction* drawEraser);
    void attachUndoRedoActions(QAction* undo, QAction* redo);

signals:
    void fogStateChanged(bool enabled);
    void gridStateChanged(bool enabled);
    void playerViewModeChanged(bool enabled);
    void fogToolModeChanged(FogToolMode mode);
    void toolChanged(ToolType tool);
    void undoRedoStateChanged(bool canUndo, bool canRedo);
    void brushSizeChanged(int size);
    void gridSizeChanged(int size);
    void fogOpacityChanged(int opacity);

private:
    QString getFogToolModeText(FogToolMode mode) const;
    QString getFogToolModeInstructions(FogToolMode mode) const;

    MainWindow* m_mainWindow;
    MapDisplay* m_mapDisplay;

    ToolType m_activeToolType;
    FogToolMode m_fogToolMode;
    bool m_gridEnabled;
    bool m_fogEnabled;
    bool m_playerViewModeEnabled;

    QSlider* m_gridSizeSlider;
    QLabel* m_gridSizeLabel;
    QSlider* m_fogBrushSlider;
    QLabel* m_fogBrushLabel;

    QAction* m_unifiedFogAction;
    QAction* m_drawPenAction;
    QAction* m_drawEraserAction;
    QAction* m_undoAction;
    QAction* m_redoAction;

    QButtonGroup* m_fogToolButtonGroup;
};

#endif // TOOLMANAGER_H