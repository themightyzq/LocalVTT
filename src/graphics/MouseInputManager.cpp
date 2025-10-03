#include "MouseInputManager.h"
#include "MapDisplay.h"
#include "FogOfWar.h"
#include "PortalSystem.h"
#include "SceneManager.h"
#include "LightingOverlay.h"
#include "GridOverlay.h"
#include "utils/FogToolMode.h"
#include "utils/ToolType.h"
#include "utils/CustomCursors.h"
#include "utils/DebugConsole.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QCursor>
#include <QScrollBar>
#include <QDateTime>
#include <QApplication>
#include <QUuid>

MouseInputManager::MouseInputManager(MapDisplay* mapDisplay, QObject* parent)
    : QObject(parent)
    , m_mapDisplay(mapDisplay)
    , m_isPanning(false)
    , m_lastMoveTime(0)
    , m_fogBrushSize(100)  // Default to 100px brush size for better visibility
    , m_fogHideModeEnabled(false)
    , m_fogRectangleModeEnabled(false)
    , m_isSelectingRectangle(false)
    , m_selectionRectIndicator(nullptr)
    , m_pointLightPlacementMode(false)
{
}

void MouseInputManager::handleMousePress(QMouseEvent* event)
{
    if (m_pointLightPlacementMode && event->button() == Qt::LeftButton) {
        QPointF scenePos = m_mapDisplay->mapToScene(event->pos());
        emit pointLightRequested(scenePos);
        return;
    }

    // Middle mouse button for panning (no modifiers needed)
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        m_velocityHistory.clear();
        m_velocityTimestamps.clear();
        m_mapDisplay->updateToolCursor();
        emit panStarted();
        return;
    }


    if (m_mapDisplay->isFogEnabled() && m_mapDisplay->getCurrentTool() == ToolType::FogBrush) {
        handleFogToolMousePress(event);
    }
}

void MouseInputManager::handleMouseMove(QMouseEvent* event)
{
    if (m_isPanning) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 timeDelta = currentTime - m_lastMoveTime;

        QPoint delta = event->pos() - m_lastPanPoint;
        emit panMoved(delta);

        if (timeDelta > 5) {
            QPointF velocity = QPointF(delta.x(), delta.y()) / timeDelta * 16.0;

            m_velocityHistory.append(velocity);
            m_velocityTimestamps.append(currentTime);

            while (!m_velocityTimestamps.isEmpty() &&
                   (currentTime - m_velocityTimestamps.first()) > 150) {
                m_velocityHistory.removeFirst();
                m_velocityTimestamps.removeFirst();
            }

            m_lastMoveTime = currentTime;
        }

        m_lastPanPoint = event->pos();
        return;
    }


    if (m_isSelectingRectangle && m_selectionRectIndicator) {
        QPointF scenePos = m_mapDisplay->mapToScene(event->pos());
        m_currentSelectionRect = QRectF(m_rectangleStartPos, scenePos).normalized();
        m_selectionRectIndicator->setRect(m_currentSelectionRect);
        m_mapDisplay->update();
        return;
    }

    // Handle fog brush dragging (only when NOT in rectangle mode)
    if (m_mapDisplay->isFogEnabled() && m_mapDisplay->getCurrentTool() == ToolType::FogBrush &&
        !m_fogRectangleModeEnabled && !m_isSelectingRectangle && (event->buttons() & Qt::LeftButton)) {
        handleFogToolMouseMove(event);
    }

    // Show brush preview only when in brush mode (not rectangle mode)
    if (m_mapDisplay->isFogEnabled() && m_mapDisplay->getCurrentTool() == ToolType::FogBrush &&
        !m_fogRectangleModeEnabled &&
        !m_isPanning && !m_isSelectingRectangle) {
        QPointF scenePos = m_mapDisplay->mapToScene(event->pos());
        m_mapDisplay->updateFogBrushPreview(scenePos, Qt::NoModifier);
        m_mapDisplay->showFogBrushPreview(true);
    } else {
        m_mapDisplay->showFogBrushPreview(false);
    }
}

void MouseInputManager::handleMouseRelease(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && m_isPanning)) {
        m_isPanning = false;
        m_mapDisplay->updateToolCursor();

        calculateReleaseVelocity();
        emit panEnded();
        return;
    }


    if (event->button() == Qt::LeftButton && m_isSelectingRectangle &&
        m_mapDisplay->getCurrentTool() == ToolType::FogBrush) {
        handleFogToolMouseRelease(event);
    }
}

void MouseInputManager::handleMouseDoubleClick(QMouseEvent* event)
{

    if (event->button() == Qt::LeftButton && m_mapDisplay->getLightingOverlay()) {
        QPointF scenePos = m_mapDisplay->mapToScene(event->pos());
        // Point light feature removed - skip double-click light handling
    }
}

void MouseInputManager::handleKeyPress(QKeyEvent* event)
{
    bool handled = false;

    {
        handled = true;

        switch(event->key()) {
            case Qt::Key_Plus:
            case Qt::Key_Equal:
                m_mapDisplay->zoomToPreset(m_mapDisplay->getZoomLevel() * 1.2);
                break;

            case Qt::Key_Minus:
            case Qt::Key_Underscore:
                m_mapDisplay->zoomToPreset(m_mapDisplay->getZoomLevel() / 1.2);
                break;

            case Qt::Key_0:
                m_mapDisplay->fitMapToView();
                break;

            case Qt::Key_1:
                if (event->modifiers() & Qt::ControlModifier) {
                    m_mapDisplay->zoomToPreset(1.0);
                } else {
                    handled = false;
                }
                break;

            case Qt::Key_2:
                if (event->modifiers() & Qt::ControlModifier) {
                    m_mapDisplay->zoomToPreset(2.0);
                } else {
                    handled = false;
                }
                break;

            case Qt::Key_3:
                if (event->modifiers() & Qt::ControlModifier) {
                    m_mapDisplay->zoomToPreset(3.0);
                } else {
                    handled = false;
                }
                break;

            case Qt::Key_4:
                if (event->modifiers() & Qt::ControlModifier) {
                    m_mapDisplay->zoomToPreset(0.5);
                } else {
                    handled = false;
                }
                break;

            case Qt::Key_5:
                m_mapDisplay->zoomToPreset(0.25);
                break;

            case Qt::Key_6:
                m_mapDisplay->zoomToPreset(1.5);
                break;

            case Qt::Key_P:
                if (m_mapDisplay->arePortalsEnabled() && m_mapDisplay->getPortalSystem()) {
                    QPointF scenePos = m_mapDisplay->mapToScene(m_mapDisplay->mapFromGlobal(QCursor::pos()));
                    emit portalToggleRequested(scenePos);
                }
                break;

            default:
                handled = false;
                break;
        }
    }

}

void MouseInputManager::setFogRectangleModeEnabled(bool enabled)
{
    m_fogRectangleModeEnabled = enabled;

    if (!enabled && m_isSelectingRectangle) {
        m_isSelectingRectangle = false;
        if (m_selectionRectIndicator) {
            QMutexLocker locker(&SceneManager::getSceneMutex());
            m_mapDisplay->getScene()->removeItem(m_selectionRectIndicator);
            delete m_selectionRectIndicator;
            m_selectionRectIndicator = nullptr;
        }
    }

    if (enabled) {
        m_mapDisplay->showFogBrushPreview(false);
    }
}



void MouseInputManager::calculateReleaseVelocity()
{
    m_panVelocity = QPointF(0, 0);

    if (m_velocityHistory.size() < 2) {
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    qreal totalWeight = 0;
    QPointF weightedVelocity(0, 0);

    for (int i = 0; i < m_velocityHistory.size(); ++i) {
        qint64 age = currentTime - m_velocityTimestamps[i];
        qreal weight = qExp(-age / 50.0);

        weightedVelocity += m_velocityHistory[i] * weight;
        totalWeight += weight;
    }

    if (totalWeight > 0) {
        m_panVelocity = weightedVelocity / totalWeight;

        const qreal MAX_VELOCITY = 50.0;
        if (m_panVelocity.manhattanLength() > MAX_VELOCITY) {
            qreal scale = MAX_VELOCITY / m_panVelocity.manhattanLength();
            m_panVelocity *= scale;
        }
    }
}

void MouseInputManager::handleFogToolMousePress(QMouseEvent* event)
{
    if (!m_mapDisplay->getFogOverlay()) return;

    // Rectangle mode controlled by button state only (no Shift key)
    if (m_fogRectangleModeEnabled && event->button() == Qt::LeftButton) {
        m_isSelectingRectangle = true;
        m_rectangleStartPos = m_mapDisplay->mapToScene(event->pos());
        m_currentSelectionRect = QRectF(m_rectangleStartPos, QSizeF(1, 1));
        m_rectangleHideMode = false;  // Always reveal (no hide mode)

        if (!m_selectionRectIndicator) {
            m_selectionRectIndicator = new QGraphicsRectItem();
            m_selectionRectIndicator->setZValue(100);

            QMutexLocker locker(&SceneManager::getSceneMutex());
            m_mapDisplay->getScene()->addItem(m_selectionRectIndicator);
        }

        // Visual feedback for rectangle mode - green for reveal
        QColor rectColor = QColor(100, 255, 100, 150);
        QPen rectPen = QPen(QColor(80, 255, 80), 2, Qt::DashLine);

        m_selectionRectIndicator->setPen(rectPen);
        m_selectionRectIndicator->setBrush(QBrush(rectColor));
        m_selectionRectIndicator->setRect(m_currentSelectionRect);
    } else if (event->button() == Qt::LeftButton) {
        // Brush mode - always reveal (no hide mode)
        QPointF scenePos = m_mapDisplay->mapToScene(event->pos());
        FogOfWar* fog = m_mapDisplay->getFogOverlay();
        fog->revealArea(scenePos, m_fogBrushSize);
        m_mapDisplay->update();
        m_mapDisplay->notifyFogChanged();
    }
}

void MouseInputManager::handleFogToolMouseMove(QMouseEvent* event)
{
    if (!m_mapDisplay->getFogOverlay()) return;

    // Only paint when dragging in brush mode (not rectangle mode)
    if (event->buttons() & Qt::LeftButton && !m_fogRectangleModeEnabled && !m_isSelectingRectangle) {
        QPointF scenePos = m_mapDisplay->mapToScene(event->pos());
        FogOfWar* fog = m_mapDisplay->getFogOverlay();

        // Always reveal (no hide mode)
        fog->revealArea(scenePos, m_fogBrushSize);
        m_mapDisplay->update();
        m_mapDisplay->notifyFogChanged();
    }
}

void MouseInputManager::handleFogToolMouseRelease(QMouseEvent* event)
{
    if (m_isSelectingRectangle && m_mapDisplay->getFogOverlay()) {
        m_isSelectingRectangle = false;

        if (!m_currentSelectionRect.isEmpty()) {
            FogOfWar* fog = m_mapDisplay->getFogOverlay();
            // Always reveal (no hide mode)
            fog->revealRectangle(m_currentSelectionRect);
            m_mapDisplay->update();
            m_mapDisplay->notifyFogChanged();
        }

        if (m_selectionRectIndicator) {
            QMutexLocker locker(&SceneManager::getSceneMutex());
            m_mapDisplay->getScene()->removeItem(m_selectionRectIndicator);
            delete m_selectionRectIndicator;
            m_selectionRectIndicator = nullptr;
        }

        m_mapDisplay->update();
    }
}






