#include "graphics/GMBeacon.h"
#include "graphics/ZLayers.h"
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
    , m_beaconColor(255, 70, 70, 255) // Red - default beacon color (brighter)
{
    setPos(position);
    setZValue(ZLayer::Beacons);

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

    // NOTE: Don't auto-start - caller must call startAnimation() after setColor()
}

GMBeacon::GMBeacon(const QPointF& position, qreal mapDimension, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_position(position)
    , m_animationProgress(0.0)
    , m_animation(nullptr)
    , m_maxRadius(mapDimension * DEFAULT_RADIUS_PERCENT)
    , m_beaconColor(255, 70, 70, 255) // Red - default beacon color (brighter)
{
    setPos(position);
    setZValue(ZLayer::Beacons);

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

    // NOTE: Don't auto-start - caller must call startAnimation() after setColor()
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

    // Calculate perceived luminance (0-1 scale) using standard coefficients
    // Bright colors (yellow, white, green) have high luminance; dark colors (red, blue, purple) have low
    qreal luminance = 0.299 * m_beaconColor.redF() + 0.587 * m_beaconColor.greenF() + 0.114 * m_beaconColor.blueF();

    // Boost opacity for darker colors - darker colors get up to 1.5x opacity boost
    // luminance=1.0 (white) -> boost=1.0, luminance=0.0 (black) -> boost=1.5
    qreal opacityBoost = 1.0 + (1.0 - luminance) * 0.5;

    // Draw multiple expanding rings - color stays vibrant, only opacity fades
    for (int ring = 0; ring < RING_COUNT; ++ring) {
        // Calculate ring-specific animation progress with delay
        qreal ringDelay = ring * 0.15; // Stagger the rings
        qreal ringProgress = qMax(0.0, (m_animationProgress - ringDelay) / (1.0 - ringDelay));

        if (ringProgress <= 0.0) continue;

        // Calculate radius for this ring
        qreal radius = ringProgress * m_maxRadius * (1.0 - ring * 0.15);

        // Calculate opacity - stay bright for first 40%, then fade to transparent
        qreal fadeStart = 0.4;  // Stay at full opacity until 40% through animation
        qreal fadeProgress = qMax(0.0, (ringProgress - fadeStart) / (1.0 - fadeStart));
        qreal baseOpacity = BASE_OPACITY * (1.0 - fadeProgress) * (1.0 - ring * 0.2);
        qreal opacity = qMin(1.0, baseOpacity * opacityBoost);  // Apply boost, cap at 1.0

        // Thick colored ring - NO black outline (that causes the "turning black" effect)
        QColor ringColor = m_beaconColor;
        ringColor.setAlphaF(opacity);
        QPen colorPen(ringColor, 5.0 - ring);
        painter->setPen(colorPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), radius, radius);

        // Filled glow for first ring
        if (ring == 0) {
            QColor fillColor = m_beaconColor;
            fillColor.setAlphaF(qMin(1.0, opacity * 0.5));  // Slightly more visible glow
            painter->setBrush(QBrush(fillColor));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(0, 0), radius * 0.5, radius * 0.5);
        }
    }

    // Draw center point that pulses - stays the selected color
    qreal pulseFactor = 0.5 + 0.5 * sin(m_animationProgress * M_PI * 4);
    qreal centerRadius = 8.0 + pulseFactor * 6.0; // Larger, more visible center
    // Center stays at full opacity for first 50%, then fades slowly
    qreal centerFadeStart = 0.5;
    qreal centerFadeProgress = qMax(0.0, (m_animationProgress - centerFadeStart) / (1.0 - centerFadeStart));
    qreal baseCenterOpacity = BASE_OPACITY * (1.0 - centerFadeProgress * 0.5); // Only fades to 50% at end
    qreal centerOpacity = qMin(1.0, baseCenterOpacity * opacityBoost);  // Apply boost to center too

    // Solid colored center - NO black outline
    QColor centerColor = m_beaconColor;
    centerColor.setAlphaF(centerOpacity);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(centerColor));
    painter->drawEllipse(QPointF(0, 0), centerRadius, centerRadius);

    // White highlight in center for visibility
    QColor highlightColor = Qt::white;
    highlightColor.setAlphaF(centerOpacity * 0.7);
    painter->setBrush(QBrush(highlightColor));
    painter->drawEllipse(QPointF(0, 0), centerRadius * 0.3, centerRadius * 0.3);
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

void GMBeacon::setColor(const QColor& color)
{
    m_beaconColor = color;
    update();
}

QList<QColor> GMBeacon::presetColors()
{
    // Full color palette - bright, high-visibility colors
    // Darker colors made brighter for better visibility
    return {
        QColor(255, 70, 70),     // Red - brighter coral-red
        QColor(100, 170, 255),   // Blue - bright sky blue
        QColor(255, 255, 0),     // Yellow - bright yellow (already high luminance)
        QColor(0, 255, 0),       // Green - bright green (already high luminance)
        QColor(255, 160, 50),    // Orange - bright orange
        QColor(220, 130, 255),   // Purple - bright lavender-purple
        QColor(255, 255, 255),   // White
        QColor(200, 200, 200),   // Grey - lighter grey for visibility
        QColor(80, 80, 80)       // Black - dark grey instead of pure black
    };
}

QString GMBeacon::colorName(const QColor& color)
{
    // Match against presets and return name
    if (color == QColor(255, 70, 70)) return "Red";
    if (color == QColor(100, 170, 255)) return "Blue";
    if (color == QColor(255, 255, 0)) return "Yellow";
    if (color == QColor(0, 255, 0)) return "Green";
    if (color == QColor(255, 160, 50)) return "Orange";
    if (color == QColor(220, 130, 255)) return "Purple";
    if (color == QColor(255, 255, 255)) return "White";
    if (color == QColor(200, 200, 200)) return "Grey";
    if (color == QColor(80, 80, 80)) return "Black";
    return "Custom";
}