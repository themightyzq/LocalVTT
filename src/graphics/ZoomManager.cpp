#include "ZoomManager.h"
#include "ZoomIndicator.h"
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPropertyAnimation>
#include <QTimer>
#include <QScrollBar>
#include <QGraphicsScene>
#include <QtMath>

ZoomManager::ZoomManager(QGraphicsView* view, QObject* parent)
    : QObject(parent)
    , m_view(view)
    , m_zoomIndicator(nullptr)
    , m_zoomFactor(1.0)
    , m_targetZoomFactor(1.0)
    , m_zoomControlsEnabled(true)
    , m_zoomAnimation(nullptr)
    , m_smoothPanTimer(nullptr)
    , m_zoomAccumulationTimer(nullptr)
    , m_animationStartZoom(1.0)
    , m_animationTargetZoom(1.0)
    , m_isZoomAnimating(false)
    , m_zoomCenterOnCursor(false)
{
    m_zoomIndicator = new ZoomIndicator(m_view);
    m_zoomIndicator->hide();

    m_zoomAccumulationTimer = new QTimer(this);
    m_zoomAccumulationTimer->setInterval(50);
    m_zoomAccumulationTimer->setSingleShot(true);
    connect(m_zoomAccumulationTimer, &QTimer::timeout, this, &ZoomManager::finishZoomAccumulation);

    m_zoomAnimation = new QPropertyAnimation(this, "", this);
    m_zoomAnimation->setDuration(200);
    m_zoomAnimation->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_zoomAnimation, &QPropertyAnimation::valueChanged, this, &ZoomManager::animateZoom);
    connect(m_zoomAnimation, &QPropertyAnimation::finished, [this]() {
        m_isZoomAnimating = false;
        m_targetZoomFactor = m_zoomFactor;
    });

    m_smoothPanTimer = new QTimer(this);
    m_smoothPanTimer->setInterval(16);
    connect(m_smoothPanTimer, &QTimer::timeout, this, &ZoomManager::updateSmoothPan);
}

ZoomManager::~ZoomManager()
{
    if (m_smoothPanTimer) {
        m_smoothPanTimer->stop();
    }
    if (m_zoomAnimation) {
        m_zoomAnimation->stop();
    }
    if (m_zoomAccumulationTimer) {
        m_zoomAccumulationTimer->stop();
    }
}

void ZoomManager::setZoomLevel(qreal zoomLevel)
{
    if (!m_view->scene()) return;

    zoomLevel = qBound(MIN_ZOOM, zoomLevel, MAX_ZOOM);

    m_view->resetTransform();
    m_view->scale(zoomLevel, zoomLevel);

    m_zoomFactor = zoomLevel;
    m_targetZoomFactor = zoomLevel;

    if (m_zoomIndicator) {
        m_zoomIndicator->showZoom(m_zoomFactor);
    }

    emit zoomChanged(m_zoomFactor);
}

void ZoomManager::zoomToPreset(qreal zoomLevel)
{
    if (!m_view->scene()) return;

    if (shouldDisableAnimation()) {
        setZoomLevel(qBound(MIN_ZOOM, zoomLevel, MAX_ZOOM));
        return;
    }

    m_targetZoomFactor = qBound(MIN_ZOOM, zoomLevel, MAX_ZOOM);

    if (m_zoomAccumulationTimer->isActive()) {
        m_zoomAccumulationTimer->stop();
    }
    m_zoomAccumulationTimer->start();
}

void ZoomManager::fitToView()
{
    if (!m_view->scene()) return;

    QList<QGraphicsItem*> items = m_view->scene()->items();
    for (QGraphicsItem* item : items) {
        if (auto* pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(item)) {
            if (pixmapItem->zValue() == 0) {
                m_view->fitInView(pixmapItem, Qt::KeepAspectRatio);
                m_zoomFactor = m_view->transform().m11();
                m_targetZoomFactor = m_zoomFactor;

                if (m_zoomIndicator) {
                    m_zoomIndicator->showZoom(m_zoomFactor);
                }

                emit zoomChanged(m_zoomFactor);
                break;
            }
        }
    }
}

void ZoomManager::animateZoomTo(qreal targetZoom, const QPointF& centerPoint)
{
    if (m_isZoomAnimating) {
        m_zoomAnimation->stop();
    }

    m_animationStartZoom = m_zoomFactor;
    m_animationTargetZoom = qBound(MIN_ZOOM, targetZoom, MAX_ZOOM);
    m_zoomAnimationCenter = centerPoint;
    m_isZoomAnimating = true;

    m_zoomAnimation->setStartValue(m_animationStartZoom);
    m_zoomAnimation->setEndValue(m_animationTargetZoom);
    m_zoomAnimation->start();
}

void ZoomManager::startSmoothPan(const QPointF& velocity)
{
    m_panVelocity = velocity;
    if (m_panVelocity.manhattanLength() > 2.0) {
        m_smoothPanTimer->start();
    }
}

void ZoomManager::stopSmoothPan()
{
    if (m_smoothPanTimer) {
        m_smoothPanTimer->stop();
    }
    m_panVelocity = QPointF(0, 0);
}

void ZoomManager::handleWheelEvent(int angleDelta, const QPointF& cursorPos)
{
    if (!m_zoomControlsEnabled || !m_view->scene()) return;

    qreal zoomDelta = angleDelta > 0 ? 1.1 : 0.9;
    qreal newZoom = m_targetZoomFactor * zoomDelta;
    newZoom = qBound(MIN_ZOOM, newZoom, MAX_ZOOM);

    if (qAbs(newZoom - m_targetZoomFactor) < 0.001) return;

    m_targetZoomFactor = newZoom;
    m_zoomCursorPos = cursorPos;
    m_zoomScenePos = m_view->mapToScene(cursorPos.toPoint());
    m_zoomCenterOnCursor = true;

    if (m_zoomAccumulationTimer->isActive()) {
        m_zoomAccumulationTimer->stop();
    }
    m_zoomAccumulationTimer->start();
}

void ZoomManager::syncZoomLevel(qreal zoomLevel, const QPointF& centerPoint)
{
    if (!m_view->scene()) return;

    m_zoomFactor = qBound(MIN_ZOOM, zoomLevel, MAX_ZOOM);
    m_targetZoomFactor = m_zoomFactor;

    m_view->resetTransform();
    m_view->scale(m_zoomFactor, m_zoomFactor);

    if (!centerPoint.isNull()) {
        m_view->centerOn(centerPoint);
    }

    if (m_zoomIndicator) {
        m_zoomIndicator->showZoom(m_zoomFactor);
    }
}

void ZoomManager::animateZoom()
{
    if (!m_zoomAnimation) return;

    qreal currentValue = m_zoomAnimation->currentValue().toReal();
    if (currentValue <= 0) return;

    m_view->resetTransform();
    m_view->scale(currentValue, currentValue);
    m_zoomFactor = currentValue;

    if (m_zoomCenterOnCursor) {
        QPointF scenePosAfterZoom = m_view->mapToScene(m_zoomCursorPos.toPoint());
        QPointF delta = scenePosAfterZoom - m_zoomScenePos;

        m_view->horizontalScrollBar()->setValue(
            m_view->horizontalScrollBar()->value() - delta.x()
        );
        m_view->verticalScrollBar()->setValue(
            m_view->verticalScrollBar()->value() - delta.y()
        );
    }

    emit zoomChanged(m_zoomFactor);
}

void ZoomManager::updateSmoothPan()
{
    const qreal FRICTION = 0.92;
    const qreal MIN_VELOCITY = 0.5;

    if (m_panVelocity.manhattanLength() < MIN_VELOCITY) {
        m_smoothPanTimer->stop();
        return;
    }

    m_view->horizontalScrollBar()->setValue(
        m_view->horizontalScrollBar()->value() - m_panVelocity.x()
    );
    m_view->verticalScrollBar()->setValue(
        m_view->verticalScrollBar()->value() - m_panVelocity.y()
    );

    m_panVelocity *= FRICTION;
}

void ZoomManager::finishZoomAccumulation()
{
    if (!shouldDisableAnimation()) {
        if (m_isZoomAnimating) {
            m_zoomAnimation->stop();
        }

        m_animationStartZoom = m_zoomFactor;
        m_animationTargetZoom = m_targetZoomFactor;
        m_isZoomAnimating = true;

        m_zoomAnimation->setStartValue(m_animationStartZoom);
        m_zoomAnimation->setEndValue(m_animationTargetZoom);
        m_zoomAnimation->start();
    } else {
        setZoomLevel(m_targetZoomFactor);
    }

    if (m_zoomIndicator) {
        m_zoomIndicator->showZoom(m_targetZoomFactor);
    }
}

bool ZoomManager::shouldDisableAnimation() const
{
    if (!m_view->scene()) return true;

    const QSizeF mapSize = m_view->scene()->sceneRect().size();
    const qreal mapArea = mapSize.width() * mapSize.height();
    return mapArea > 2.0e7;
}