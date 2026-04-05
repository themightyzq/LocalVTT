#include "FogToolsController.h"
#include "graphics/MapDisplay.h"
#include "utils/FogToolMode.h"
#include "graphics/FogOfWar.h"
#include "ui/DarkTheme.h"
#include "utils/ToolType.h"
#include <QToolBar>
#include <QToolButton>
#include <QActionGroup>

FogToolsController::FogToolsController(QObject* parent)
    : QObject(parent)
    , m_mode(FogToolMode::UnifiedFog)
{
}

FogToolsController::FogToolbarActions FogToolsController::createToolbarActions(QToolBar* toolbar, QAction* fogToggleAction)
{
    FogToolbarActions actions;

    // Fog Mode toggle (action provided by caller via ActionRegistry)
    if (fogToggleAction) {
        fogToggleAction->setIcon(QIcon(":/icons/fog.svg"));
        fogToggleAction->setText("Fog Mode");
        fogToggleAction->setCheckable(true);
        fogToggleAction->setChecked(false);
        fogToggleAction->setToolTip("<b>Fog Mode</b><br>Enable Fog of War<br>Map goes black until revealed<br><i>Shortcut: F</i>");
        toolbar->addAction(fogToggleAction);
        actions.fogModeToggle = fogToggleAction;
    }

    // Reveal/Hide toggle
    actions.hideToggle = new QAction("REVEAL", toolbar);
    actions.hideToggle->setCheckable(true);
    actions.hideToggle->setChecked(false);
    actions.hideToggle->setEnabled(false);
    actions.hideToggle->setToolTip("<b>Reveal/Hide Mode</b><br>Toggle between revealing and hiding fog<br><i>Shortcut: H</i>");
    toolbar->addAction(actions.hideToggle);

    // Force text display (overrides toolbar-wide icon-only mode)
    if (auto* btn = qobject_cast<QToolButton*>(toolbar->widgetForAction(actions.hideToggle))) {
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setStyleSheet(DarkTheme::successButtonStyle());
    }

    // Brush tool
    actions.brushTool = new QAction(QIcon(":/icons/fog_brush.svg"), "Reveal Brush", toolbar);
    actions.brushTool->setCheckable(true);
    actions.brushTool->setEnabled(false);
    actions.brushTool->setToolTip("<b>Reveal Brush</b><br>Circle brush tool<br>Click and drag to reveal areas");
    toolbar->addAction(actions.brushTool);

    // Rectangle tool
    actions.rectTool = new QAction(QIcon(":/icons/fog_rect.svg"), "Reveal Rectangle", toolbar);
    actions.rectTool->setCheckable(true);
    actions.rectTool->setEnabled(false);
    actions.rectTool->setToolTip("<b>Reveal Rectangle</b><br>Rectangle selection tool<br>Drag to reveal rectangular areas");
    toolbar->addAction(actions.rectTool);

    // Mutual exclusivity for brush/rect
    auto* fogToolGroup = new QActionGroup(toolbar);
    fogToolGroup->setExclusive(true);
    fogToolGroup->addAction(actions.brushTool);
    fogToolGroup->addAction(actions.rectTool);

    // Reset Fog
    actions.resetFog = new QAction(QIcon(":/icons/reset.svg"), "Reset Fog", toolbar);
    actions.resetFog->setToolTip("<b>Reset Fog</b><br>Clear all fog and start over<br><span style='color: #ff6b6b;'>&#x26A0; Requires confirmation</span>");
    actions.resetFog->setEnabled(false);
    toolbar->addAction(actions.resetFog);

    // Style Reset Fog as danger
    if (auto* btn = qobject_cast<QToolButton*>(toolbar->widgetForAction(actions.resetFog))) {
        btn->setStyleSheet(DarkTheme::dangerButtonStyle());
    }

    // Lock Fog
    actions.lockFog = new QAction(QIcon(":/icons/unlock.svg"), "Lock Fog", toolbar);
    actions.lockFog->setToolTip("<b>Fog Unlocked</b><br>Click to lock fog editing");
    actions.lockFog->setCheckable(true);
    actions.lockFog->setChecked(false);
    toolbar->addAction(actions.lockFog);

    return actions;
}

void FogToolsController::setDisplay(MapDisplay* display)
{
    m_display = display;
    if (m_display) {
        m_display->setFogBrushSize(m_brushSize);
        applyMode();
    }
}

void FogToolsController::setMode(FogToolMode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    applyMode();

    // Update unified fog action check state if available
    if (m_revealRectAction) {
        m_revealRectAction->setChecked(m_mode == FogToolMode::UnifiedFog);
    }
}

void FogToolsController::setBrushSize(int px)
{
    m_brushSize = px;
    if (m_display) {
        m_display->setFogBrushSize(m_brushSize);
    }
    if (m_brushLabel) {
        m_brushLabel->setText(QString("Brush: %1px").arg(m_brushSize));
    }
}

void FogToolsController::applyMode()
{
    if (!m_display) return;

    // In unified mode, behavior is determined by modifier keys at runtime
    // No need to preset hide/rectangle modes
    if (m_mode == FogToolMode::UnifiedFog) {
        m_display->setFogHideModeEnabled(false);
        m_display->setFogRectangleModeEnabled(false);
    }

    m_display->updateToolCursor();
}

void FogToolsController::attachUI(QAction* unifiedFogAction,
                                  QSlider* brushSlider,
                                  QLabel* brushLabel,
                                  QSlider* gmOpacitySlider,
                                  QLabel* gmOpacityLabel)
{
    m_revealRectAction = unifiedFogAction;  // Reuse existing member for unified action
    m_brushSlider = brushSlider;
    m_brushLabel = brushLabel;
    m_gmOpacitySlider = gmOpacitySlider;
    m_gmOpacityLabel = gmOpacityLabel;

    if (m_revealRectAction) {
        QObject::connect(m_revealRectAction, &QAction::triggered, this, [this](){ emit modeRequested(FogToolMode::UnifiedFog); });
    }

    if (m_brushSlider) {
        QObject::connect(m_brushSlider, &QSlider::valueChanged, this, [this](int v){ setBrushSize(v); });
    }
    if (m_gmOpacitySlider) {
        QObject::connect(m_gmOpacitySlider, &QSlider::valueChanged, this, [this](int v){
            if (m_gmOpacityLabel) m_gmOpacityLabel->setText(QString("%1% DM").arg(v));
            if (m_display && m_display->getFogOverlay()) {
                m_display->getFogOverlay()->setGMOpacity(v / 100.0);
            }
        });
    }

    // Initialize UI state
    setMode(m_mode);
    setBrushSize(m_brushSize);

    // Provide icon for unified fog action
    if (m_revealRectAction) m_revealRectAction->setIcon(QIcon(":/icons/reveal_circle.svg"));
}
