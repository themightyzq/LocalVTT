#include "graphics/PingIndicator.h"
#include <QPen>
#include <QBrush>
#include <QGraphicsScene>

PingIndicator::PingIndicator(const QPointF& position, QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
    , m_opacityAnimation(new QPropertyAnimation(this, "opacity", this))
{
    setRect(-PING_RADIUS, -PING_RADIUS, PING_RADIUS * 2, PING_RADIUS * 2);
    setPos(position);

    QPen pen(Qt::cyan, 4.0);
    setBrush(QBrush(QColor(0, 255, 255, 80)));
    setPen(pen);

    setZValue(10);
    setOpacity(1.0);

    m_opacityAnimation->setDuration(ANIMATION_DURATION);
    m_opacityAnimation->setStartValue(1.0);
    m_opacityAnimation->setEndValue(0.0);

    connect(m_opacityAnimation, &QPropertyAnimation::finished,
            this, &PingIndicator::onAnimationFinished);
}

void PingIndicator::startAnimation()
{
    m_opacityAnimation->start();
}

void PingIndicator::onAnimationFinished()
{
    if (scene()) {
        scene()->removeItem(this);
    }
    deleteLater();
}