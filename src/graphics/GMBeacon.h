#ifndef GMBEACON_H
#define GMBEACON_H

#include <QGraphicsItem>
#include <QPropertyAnimation>
#include <QTimer>
#include <QColor>

class GMBeacon : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_PROPERTY(qreal animationProgress READ animationProgress WRITE setAnimationProgress)
    Q_INTERFACES(QGraphicsItem)

public:
    explicit GMBeacon(const QPointF& position, QGraphicsItem* parent = nullptr);
    explicit GMBeacon(const QPointF& position, qreal viewportWidth, QGraphicsItem* parent = nullptr);
    ~GMBeacon();

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    // Animation control
    void startAnimation();
    qreal animationProgress() const { return m_animationProgress; }
    void setAnimationProgress(qreal progress);

signals:
    void animationFinished();

private:
    QPointF m_position;
    qreal m_animationProgress;
    QPropertyAnimation* m_animation;

    // Visual parameters
    qreal m_maxRadius;
    static constexpr int RING_COUNT = 3;
    static constexpr int ANIMATION_DURATION = 2000; // 2 seconds
    static constexpr qreal BASE_OPACITY = 0.9;
    static constexpr qreal DEFAULT_RADIUS_PERCENT = 0.20; // 20% of viewport width

    QColor m_beaconColor;
};

#endif // GMBEACON_H