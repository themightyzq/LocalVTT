#include "graphics/GMBeacon.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QGraphicsScene>
#include <cmath>

GMBeacon::GMBeacon(const QPointF& position, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_position(position)
    , m_animationProgress(0.0)
    , m_animation(nullptr)
    , m_maxRadius(60.0) // Default fallback radius
    , m_beaconColor(0, 255, 255, 255) // Bright cyan for high visibility
{
    setPos(position);
    setZValue(1000); // Above everything else

    // Create the animation
    m_animation = new QPropertyAnimation(this, "animationProgress");
    m_animation->setDuration(ANIMATION_DURATION);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Auto-delete when animation finishes
    connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
        emit animationFinished();
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
    });

    startAnimation();
}

GMBeacon::GMBeacon(const QPointF& position, qreal viewportWidth, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_position(position)
    , m_animationProgress(0.0)
    , m_animation(nullptr)
    , m_maxRadius(viewportWidth * DEFAULT_RADIUS_PERCENT)
    , m_beaconColor(0, 255, 255, 255) // Bright cyan for high visibility
{
    setPos(position);
    setZValue(1000); // Above everything else

    // Create the animation
    m_animation = new QPropertyAnimation(this, "animationProgress");
    m_animation->setDuration(ANIMATION_DURATION);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Auto-delete when animation finishes
    connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
        emit animationFinished();
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
    });

    startAnimation();
}

GMBeacon::~GMBeacon()
{
    if (m_animation) {
        m_animation->stop();
        delete m_animation;
    }
}

QRectF GMBeacon::boundingRect() const
{
    // Bounding rect needs to cover the maximum radius
    return QRectF(-m_maxRadius, -m_maxRadius, m_maxRadius * 2, m_maxRadius * 2);
}

void GMBeacon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Draw multiple expanding rings for a more dynamic effect
    for (int ring = 0; ring < RING_COUNT; ++ring) {
        // Calculate ring-specific animation progress with delay
        qreal ringDelay = ring * 0.15; // Stagger the rings
        qreal ringProgress = qMax(0.0, (m_animationProgress - ringDelay) / (1.0 - ringDelay));

        if (ringProgress <= 0.0) continue;

        // Calculate radius for this ring
        qreal radius = ringProgress * m_maxRadius * (1.0 - ring * 0.15);

        // Calculate opacity (fade out as it expands)
        qreal opacity = BASE_OPACITY * (1.0 - ringProgress) * (1.0 - ring * 0.3);

        // Draw the ring with strong outline
        QColor ringColor = m_beaconColor;
        ringColor.setAlphaF(opacity);

        // Strong black outline for contrast
        QPen outlinePen(Qt::black, 6.0 - ring);
        painter->setPen(outlinePen);
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), radius, radius);

        // Bright colored ring
        QPen colorPen(ringColor, 4.0 - ring);
        painter->setPen(colorPen);
        painter->drawEllipse(QPointF(0, 0), radius, radius);

        // Inner filled circle (only for the first ring)
        if (ring == 0) {
            QColor fillColor = m_beaconColor;
            fillColor.setAlphaF(opacity * 0.3);
            painter->setBrush(QBrush(fillColor));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(0, 0), radius * 0.3, radius * 0.3);
        }
    }

    // Draw center point that pulses with strong contrast
    qreal pulseFactor = 0.5 + 0.5 * sin(m_animationProgress * M_PI * 4);
    qreal centerRadius = 6.0 + pulseFactor * 4.0; // Larger center point
    qreal centerOpacity = BASE_OPACITY * (1.0 - m_animationProgress * 0.5); // Less fade

    // Black outline for center
    painter->setPen(QPen(Qt::black, 3.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(0, 0), centerRadius + 2, centerRadius + 2);

    // Bright colored center
    QColor centerColor = m_beaconColor;
    centerColor.setAlphaF(centerOpacity);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(centerColor));
    painter->drawEllipse(QPointF(0, 0), centerRadius, centerRadius);

    // Bright white core
    QColor brightColor = Qt::white;
    brightColor.setAlphaF(centerOpacity);
    painter->setBrush(QBrush(brightColor));
    painter->drawEllipse(QPointF(0, 0), centerRadius * 0.4, centerRadius * 0.4);
}

void GMBeacon::startAnimation()
{
    if (m_animation) {
        m_animation->start();
    }
}

void GMBeacon::setAnimationProgress(qreal progress)
{
    m_animationProgress = progress;
    update();
}