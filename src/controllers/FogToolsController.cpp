#include "FogToolsController.h"
#include "graphics/MapDisplay.h"
#include "utils/FogToolMode.h"
#include "graphics/FogOfWar.h"

FogToolsController::FogToolsController(QObject* parent)
    : QObject(parent)
    , m_mode(FogToolMode::UnifiedFog)
{
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
