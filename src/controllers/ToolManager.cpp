#include "controllers/ToolManager.h"
#include "ui/MainWindow.h"
#include "graphics/MapDisplay.h"
#include "graphics/FogOfWar.h"
#include "graphics/GridOverlay.h"
#include "utils/DebugConsole.h"
#include <QAction>
#include <QSlider>
#include <QLabel>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QMessageBox>
#include <QStatusBar>
#include <iostream>

ToolManager::ToolManager(MainWindow* mainWindow, MapDisplay* mapDisplay, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_mapDisplay(mapDisplay)
    , m_activeToolType(ToolType::Pointer)
    , m_fogToolMode(FogToolMode::UnifiedFog)
    , m_gridEnabled(true)
    , m_fogEnabled(false)
    , m_playerViewModeEnabled(false)
    , m_gridSizeSlider(nullptr)
    , m_gridSizeLabel(nullptr)
    , m_fogBrushSlider(nullptr)
    , m_fogBrushLabel(nullptr)
    , m_unifiedFogAction(nullptr)
    , m_drawPenAction(nullptr)
    , m_drawEraserAction(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_fogToolButtonGroup(nullptr)
{
}

void ToolManager::setupFogToolModeSystem()
{
    m_fogToolButtonGroup = new QButtonGroup(m_mainWindow);
    m_fogToolButtonGroup->setExclusive(true);

    // Note: FogToolMode is managed by MainWindow, not MapDisplay
}

void ToolManager::setActiveTool(ToolType tool)
{
    std::cerr << "ToolManager::setActiveTool called with tool: " << static_cast<int>(tool) << std::endl;
    std::cerr.flush();

    if (m_activeToolType == tool) {
        std::cerr << "ToolManager: Tool already active, returning" << std::endl;
        std::cerr.flush();
        return;
    }

    m_activeToolType = tool;

    switch (tool) {
        case ToolType::Pointer:
            std::cerr << "ToolManager: Switching to Pointer tool" << std::endl;
            break;
        case ToolType::FogBrush:
            std::cerr << "ToolManager: Switching to FogBrush tool" << std::endl;
            break;
        case ToolType::FogRectangle:
            std::cerr << "ToolManager: Switching to FogRectangle tool" << std::endl;
            break;
    }
    std::cerr.flush();

    emit toolChanged(tool);

    DebugConsole::info(
        QString("Active tool changed to: %1").arg(getActiveToolName()), "ToolManager");
}

QString ToolManager::getActiveToolName() const
{
    switch (m_activeToolType) {
        case ToolType::Pointer: return "Pointer";
        case ToolType::FogBrush: return "Fog Brush";
        case ToolType::FogRectangle: return "Fog Rectangle";
        default: return "Unknown";
    }
}

bool ToolManager::handleEscapeKey()
{
    // Escape returns to default Pointer tool (no Pan tool per CLAUDE.md)
    if (m_activeToolType != ToolType::Pointer) {
        setActiveTool(ToolType::Pointer);
        return true;
    }
    return false;
}

void ToolManager::setFogToolMode(FogToolMode mode)
{
    m_fogToolMode = mode;

    if (m_mainWindow) {
        m_mainWindow->setFogToolMode(mode);
    }

    updateFogToolModeUI();
    updateFogToolModeStatus();
    emit fogToolModeChanged(mode);

    DebugConsole::info(
        QString("Fog tool mode changed to: %1").arg(getFogToolModeText(mode)), "ToolManager");
}

void ToolManager::toggleFogOfWar()
{
    m_fogEnabled = !m_fogEnabled;

    if (m_mapDisplay) {
        m_mapDisplay->setFogEnabled(m_fogEnabled);
    }

    emit fogStateChanged(m_fogEnabled);

    DebugConsole::info(
        QString("Fog of War %1").arg(m_fogEnabled ? "enabled" : "disabled"), "ToolManager");
}

void ToolManager::clearFogOfWar()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        m_mainWindow,
        "Clear Fog of War",
        "This will reveal the entire map to players. Are you sure?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_mapDisplay->clearFog();
        DebugConsole::info("Fog of War cleared", "ToolManager");
    }
}

void ToolManager::resetFogOfWar()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        m_mainWindow,
        "Reset Fog of War",
        "This will hide the entire map from players. Are you sure?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_mapDisplay->resetFog();
        DebugConsole::info("Fog of War reset", "ToolManager");
    }
}

void ToolManager::toggleGrid()
{
    m_gridEnabled = !m_gridEnabled;

    if (m_mapDisplay) {
        m_mapDisplay->setGridEnabled(m_gridEnabled);
    }

    emit gridStateChanged(m_gridEnabled);

    DebugConsole::info(
        QString("Grid %1").arg(m_gridEnabled ? "enabled" : "disabled"), "ToolManager");
}

void ToolManager::toggleGridType()
{
    // Grid type switching removed - only square grids supported now
    if (!m_mapDisplay || !m_mapDisplay->getGridOverlay()) return;

    DebugConsole::info("Grid type is square (only type supported)", "ToolManager");
}

void ToolManager::showGridInfo()
{
    if (!m_mapDisplay || !m_mapDisplay->getGridOverlay()) return;

    int gridSize = m_mapDisplay->getGridOverlay()->getGridSize();

    QString info = QString("Grid Information:\n\n"
                          "Type: Square\n"
                          "Size: %1 pixels\n"
                          "D&D Scale: 5 feet per square")
                          .arg(gridSize);

    QMessageBox::information(m_mainWindow, "Grid Information", info);
}

void ToolManager::setStandardGrid()
{
    if (!m_mapDisplay || !m_mapDisplay->getGridOverlay()) return;

    int standardSize = GridOverlay::calculateDnDGridSize();
    m_mapDisplay->getGridOverlay()->setGridSize(standardSize);

    updateGridSizeSlider();

    DebugConsole::info(
        QString("Grid reset to D&D standard: %1 pixels").arg(standardSize), "ToolManager");
}



void ToolManager::togglePlayerViewMode()
{
    m_playerViewModeEnabled = !m_playerViewModeEnabled;

    if (m_mapDisplay) {
        // Player view mode is handled by MainWindow
    }

    emit playerViewModeChanged(m_playerViewModeEnabled);

    DebugConsole::info(
        QString("Player view mode %1").arg(m_playerViewModeEnabled ? "enabled" : "disabled"), "ToolManager");
}

void ToolManager::undoFogChange()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) return;

    if (m_mapDisplay->getFogOverlay()->canUndo()) {
        m_mapDisplay->getFogOverlay()->undo();
        updateUndoRedoButtons();
        DebugConsole::info("Fog change undone", "ToolManager");
    }
}

void ToolManager::redoFogChange()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) return;

    if (m_mapDisplay->getFogOverlay()->canRedo()) {
        m_mapDisplay->getFogOverlay()->redo();
        updateUndoRedoButtons();
        DebugConsole::info("Fog change redone", "ToolManager");
    }
}

void ToolManager::setSmallBrush()
{
    if (m_fogBrushSlider) {
        m_fogBrushSlider->setValue(30);
    }
}

void ToolManager::setMediumBrush()
{
    if (m_fogBrushSlider) {
        m_fogBrushSlider->setValue(60);
    }
}

void ToolManager::setLargeBrush()
{
    if (m_fogBrushSlider) {
        m_fogBrushSlider->setValue(100);
    }
}

void ToolManager::onGridSizeChanged(int value)
{
    if (!m_mapDisplay || !m_mapDisplay->getGridOverlay()) return;

    m_mapDisplay->getGridOverlay()->setGridSize(value);

    if (m_gridSizeLabel) {
        m_gridSizeLabel->setText(QString("Grid: %1px").arg(value));
    }

    emit gridSizeChanged(value);
}

void ToolManager::onFogBrushSizeChanged(int value)
{
    if (m_mapDisplay) {
        m_mapDisplay->setFogBrushSize(value);
    }

    if (m_fogBrushLabel) {
        m_fogBrushLabel->setText(QString("Brush: %1px").arg(value));
    }

    emit brushSizeChanged(value);
}

void ToolManager::onGMOpacityChanged(int value)
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) return;

    qreal opacity = value / 100.0;
    m_mapDisplay->getFogOverlay()->setGMOpacity(opacity);

    emit fogOpacityChanged(value);
}

void ToolManager::updateGridSizeSlider()
{
    if (!m_gridSizeSlider || !m_mapDisplay || !m_mapDisplay->getGridOverlay()) return;

    int currentSize = m_mapDisplay->getGridOverlay()->getGridSize();
    m_gridSizeSlider->setValue(currentSize);
}

void ToolManager::updateUndoRedoButtons()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) return;

    bool canUndo = m_mapDisplay->getFogOverlay()->canUndo();
    bool canRedo = m_mapDisplay->getFogOverlay()->canRedo();

    if (m_undoAction) {
        m_undoAction->setEnabled(canUndo);
    }

    if (m_redoAction) {
        m_redoAction->setEnabled(canRedo);
    }

    emit undoRedoStateChanged(canUndo, canRedo);
}

void ToolManager::updateFogToolModeUI()
{
    if (!m_fogToolButtonGroup) return;

    QAbstractButton* button = m_fogToolButtonGroup->button(static_cast<int>(m_fogToolMode));
    if (button) {
        button->setChecked(true);
    }

    // Update action states based on new unified fog tool mode
    if (m_unifiedFogAction) {
        m_unifiedFogAction->setChecked(m_fogToolMode == FogToolMode::UnifiedFog);
    }
    if (m_drawPenAction) {
        m_drawPenAction->setChecked(m_fogToolMode == FogToolMode::DrawPen);
    }
    if (m_drawEraserAction) {
        m_drawEraserAction->setChecked(m_fogToolMode == FogToolMode::DrawEraser);
    }
}

void ToolManager::updateFogToolModeStatus()
{
    QString modeText = getFogToolModeText(m_fogToolMode);
    QString instructions = getFogToolModeInstructions(m_fogToolMode);

    if (m_mainWindow) {
        m_mainWindow->statusBar()->showMessage(instructions, 3000);
    }
}

QString ToolManager::getFogToolModeText(FogToolMode mode) const
{
    switch (mode) {
        case FogToolMode::UnifiedFog: return "Unified Fog";
        case FogToolMode::DrawPen: return "Draw Pen";
        case FogToolMode::DrawEraser: return "Draw Eraser";
        default: return "Unknown";
    }
}

QString ToolManager::getFogToolModeInstructions(FogToolMode mode) const
{
    switch (mode) {
        case FogToolMode::UnifiedFog:
            return "Click to reveal, Shift+Click to hide, Alt for rectangle";
        case FogToolMode::DrawPen:
            return "Click and drag to draw with pen";
        case FogToolMode::DrawEraser:
            return "Click and drag to erase";
        default:
            return "";
    }
}

void ToolManager::attachGridSlider(QSlider* slider, QLabel* label)
{
    m_gridSizeSlider = slider;
    m_gridSizeLabel = label;

    if (m_gridSizeSlider) {
        connect(m_gridSizeSlider, &QSlider::valueChanged,
                this, &ToolManager::onGridSizeChanged);
    }
}

void ToolManager::attachFogBrushSlider(QSlider* slider, QLabel* label)
{
    m_fogBrushSlider = slider;
    m_fogBrushLabel = label;

    if (m_fogBrushSlider) {
        connect(m_fogBrushSlider, &QSlider::valueChanged,
                this, &ToolManager::onFogBrushSizeChanged);
    }
}

void ToolManager::attachFogToolActions(QAction* unifiedFog, QAction* drawPen, QAction* drawEraser)
{
    m_unifiedFogAction = unifiedFog;
    m_drawPenAction = drawPen;
    m_drawEraserAction = drawEraser;

    if (m_unifiedFogAction) {
        connect(m_unifiedFogAction, &QAction::triggered,
                [this]() { setFogToolMode(FogToolMode::UnifiedFog); });
    }
    if (m_drawPenAction) {
        connect(m_drawPenAction, &QAction::triggered,
                [this]() { setFogToolMode(FogToolMode::DrawPen); });
    }
    if (m_drawEraserAction) {
        connect(m_drawEraserAction, &QAction::triggered,
                [this]() { setFogToolMode(FogToolMode::DrawEraser); });
    }
}

void ToolManager::attachUndoRedoActions(QAction* undo, QAction* redo)
{
    m_undoAction = undo;
    m_redoAction = redo;

    if (m_undoAction) {
        connect(m_undoAction, &QAction::triggered,
                this, &ToolManager::undoFogChange);
    }
    if (m_redoAction) {
        connect(m_redoAction, &QAction::triggered,
                this, &ToolManager::redoFogChange);
    }

    updateUndoRedoButtons();
}