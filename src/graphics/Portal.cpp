#include "graphics/Portal.h"
#include "utils/DebugConsole.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtMath>

Portal::Portal(const QPointF& position, const QPointF& bound1, const QPointF& bound2,
               qreal rotation, bool closed, bool freestanding)
    : QGraphicsItem()
    , m_position(position)
    , m_bound1(bound1)
    , m_bound2(bound2)
    , m_rotation(rotation)
    , m_closed(closed)
    , m_freestanding(freestanding)
    , m_highlighted(false)
    , m_pixelsPerGrid(50)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptHoverEvents(true);
    setZValue(10);
}

Portal::~Portal()
{
}

QRectF Portal::boundingRect() const
{
    return calculateBoundingRect();
}

void Portal::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!painter) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);

    QPointF pixelPos = gridToPixel(m_position);
    QPointF pixelBound1 = gridToPixel(m_bound1);
    QPointF pixelBound2 = gridToPixel(m_bound2);

    QLineF portalLine(pixelBound1, pixelBound2);

    if (portalLine.length() < 1.0) {
        return;
    }

    QPen portalPen = getPortalPen();
    QBrush portalBrush = getPortalBrush();

    painter->setPen(portalPen);
    painter->setBrush(portalBrush);

    const qreal portalWidth = 8.0;
    const qreal halfWidth = portalWidth * 0.5;

    QPointF perpendicular = QPointF(-portalLine.dy(), portalLine.dx());
    qreal length = qSqrt(perpendicular.x() * perpendicular.x() + perpendicular.y() * perpendicular.y());
    if (length > 0.001) {
        perpendicular = QPointF(perpendicular.x() / length * halfWidth, perpendicular.y() / length * halfWidth);
    }

    QPolygonF portalRect;
    portalRect << pixelBound1 + perpendicular
               << pixelBound1 - perpendicular
               << pixelBound2 - perpendicular
               << pixelBound2 + perpendicular;

    painter->drawPolygon(portalRect);

    if (m_closed) {
        const int crossCount = qMax(1, (int)(portalLine.length() / 15.0));
        QPen crossPen(QColor(220, 60, 60), 2.0);
        painter->setPen(crossPen);

        for (int i = 0; i < crossCount; ++i) {
            qreal t = (i + 0.5) / crossCount;
            QPointF crossCenter = pixelBound1 + t * (pixelBound2 - pixelBound1);

            const qreal crossSize = halfWidth * 0.8;
            QPointF crossPerp = QPointF(perpendicular.x() / halfWidth * crossSize,
                                       perpendicular.y() / halfWidth * crossSize);

            painter->drawLine(crossCenter - crossPerp, crossCenter + crossPerp);
        }
    }

    if (m_highlighted) {
        QPen highlightPen(QColor(255, 255, 100, 180), 3.0);
        painter->setPen(highlightPen);
        painter->setBrush(Qt::NoBrush);

        QRectF highlightRect = portalRect.boundingRect().adjusted(-2, -2, 2, 2);
        painter->drawRect(highlightRect);
    }
}

void Portal::setOpen(bool open)
{
    if (m_closed == open) {
        m_closed = !open;
        update();
        DebugConsole::info(QString("Portal state changed to: %1").arg(m_closed ? "closed" : "open"), "Portal");
    }
}

void Portal::toggleState()
{
    setOpen(m_closed);
}

bool Portal::containsPoint(const QPointF& point) const
{
    QPointF pixelPoint = gridToPixel(point);
    QPointF pixelBound1 = gridToPixel(m_bound1);
    QPointF pixelBound2 = gridToPixel(m_bound2);

    QLineF portalLine(pixelBound1, pixelBound2);
    const qreal portalWidth = 8.0;

    QPointF closestPoint;
    qreal t = QPointF::dotProduct(pixelPoint - pixelBound1, portalLine.p2() - portalLine.p1()) /
              QPointF::dotProduct(portalLine.p2() - portalLine.p1(), portalLine.p2() - portalLine.p1());

    t = qBound(0.0, t, 1.0);
    closestPoint = pixelBound1 + t * (pixelBound2 - pixelBound1);

    qreal distance = QLineF(pixelPoint, closestPoint).length();
    return distance <= portalWidth;
}

qreal Portal::distanceToPoint(const QPointF& point) const
{
    QPointF pixelPoint = gridToPixel(point);
    QPointF pixelBound1 = gridToPixel(m_bound1);
    QPointF pixelBound2 = gridToPixel(m_bound2);

    QLineF portalLine(pixelBound1, pixelBound2);

    QPointF closestPoint;
    qreal t = QPointF::dotProduct(pixelPoint - pixelBound1, portalLine.p2() - portalLine.p1()) /
              QPointF::dotProduct(portalLine.p2() - portalLine.p1(), portalLine.p2() - portalLine.p1());

    t = qBound(0.0, t, 1.0);
    closestPoint = pixelBound1 + t * (pixelBound2 - pixelBound1);

    return QLineF(pixelPoint, closestPoint).length();
}

void Portal::setHighlighted(bool highlighted)
{
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        update();
    }
}

QPointF Portal::gridToPixel(const QPointF& gridPos) const
{
    return QPointF(gridPos.x() * m_pixelsPerGrid, gridPos.y() * m_pixelsPerGrid);
}

QRectF Portal::calculateBoundingRect() const
{
    QPointF pixelBound1 = gridToPixel(m_bound1);
    QPointF pixelBound2 = gridToPixel(m_bound2);

    qreal minX = qMin(pixelBound1.x(), pixelBound2.x());
    qreal maxX = qMax(pixelBound1.x(), pixelBound2.x());
    qreal minY = qMin(pixelBound1.y(), pixelBound2.y());
    qreal maxY = qMax(pixelBound1.y(), pixelBound2.y());

    const qreal margin = 15.0;
    return QRectF(minX - margin, minY - margin,
                  maxX - minX + 2 * margin, maxY - minY + 2 * margin);
}

QColor Portal::getPortalColor() const
{
    if (m_closed) {
        return QColor(180, 100, 100, 200);
    } else {
        return QColor(100, 180, 100, 200);
    }
}

QPen Portal::getPortalPen() const
{
    QColor penColor = getPortalColor();
    penColor.setAlpha(255);
    return QPen(penColor, 2.0);
}

QBrush Portal::getPortalBrush() const
{
    return QBrush(getPortalColor());
}