#ifndef PORTAL_H
#define PORTAL_H

#include <QGraphicsItem>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <QPen>
#include <QBrush>

class Portal : public QGraphicsItem
{
public:
    Portal(const QPointF& position, const QPointF& bound1, const QPointF& bound2,
           qreal rotation = 0.0, bool closed = false, bool freestanding = false);
    ~Portal();

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    bool isOpen() const { return !m_closed; }
    bool isClosed() const { return m_closed; }
    void setOpen(bool open);
    void toggleState();

    QPointF getPosition() const { return m_position; }
    QPointF getBound1() const { return m_bound1; }
    QPointF getBound2() const { return m_bound2; }
    qreal getRotation() const { return m_rotation; }
    bool isFreestanding() const { return m_freestanding; }

    bool containsPoint(const QPointF& point) const;
    qreal distanceToPoint(const QPointF& point) const;

    void setPixelsPerGrid(int pixelsPerGrid) { m_pixelsPerGrid = pixelsPerGrid; update(); }
    int getPixelsPerGrid() const { return m_pixelsPerGrid; }

    void setHighlighted(bool highlighted);
    bool isHighlighted() const { return m_highlighted; }

    enum PortalState {
        Open,
        Closed
    };

    PortalState getState() const { return m_closed ? Closed : Open; }

private:
    QPointF m_position;
    QPointF m_bound1;
    QPointF m_bound2;
    qreal m_rotation;
    bool m_closed;
    bool m_freestanding;
    bool m_highlighted;
    int m_pixelsPerGrid;

    QPointF gridToPixel(const QPointF& gridPos) const;
    QRectF calculateBoundingRect() const;
    QColor getPortalColor() const;
    QPen getPortalPen() const;
    QBrush getPortalBrush() const;
};

#endif // PORTAL_H