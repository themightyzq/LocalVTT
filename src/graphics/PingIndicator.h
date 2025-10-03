#ifndef PINGINDICATOR_H
#define PINGINDICATOR_H

#include <QGraphicsEllipseItem>
#include <QPropertyAnimation>
#include <QObject>

class PingIndicator : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit PingIndicator(const QPointF& position, QGraphicsItem* parent = nullptr);

    void startAnimation();

private slots:
    void onAnimationFinished();

private:
    QPropertyAnimation* m_opacityAnimation;
    static constexpr qreal PING_RADIUS = 30.0;
    static constexpr int ANIMATION_DURATION = 3000;
};

#endif // PINGINDICATOR_H