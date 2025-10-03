#ifndef PORTALSYSTEM_H
#define PORTALSYSTEM_H

#include <QGraphicsItem>
#include <QPointF>
#include <QList>
#include <QMutex>
#include <QSize>
#include "graphics/Portal.h"

class PortalSystem : public QGraphicsItem
{
public:
    struct PortalData {
        QPointF position;
        QPointF bound1;
        QPointF bound2;
        qreal rotation;
        bool closed;
        bool freestanding;

        PortalData() : rotation(0.0), closed(false), freestanding(false) {}
        PortalData(const QPointF& pos, const QPointF& b1, const QPointF& b2,
                   qreal rot = 0.0, bool cls = false, bool fs = false)
            : position(pos), bound1(b1), bound2(b2), rotation(rot), closed(cls), freestanding(fs) {}
    };

    PortalSystem();
    ~PortalSystem();

    void setMapSize(const QSize& size);
    void setPortals(const QList<PortalData>& portals);
    void clearPortals();

    Portal* findNearestPortal(const QPointF& point, qreal maxDistance = 50.0) const;
    bool togglePortalAt(const QPointF& point, qreal maxDistance = 50.0);
    void setAllPortalsOpen(bool open);

    void setPixelsPerGrid(int pixelsPerGrid);
    int getPixelsPerGrid() const { return m_pixelsPerGrid; }

    const QList<Portal*>& getPortals() const { return m_portals; }

    bool isPortalBlocking(const QPointF& start, const QPointF& end) const;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QSize m_mapSize;
    QList<Portal*> m_portals;
    int m_pixelsPerGrid;

    mutable QMutex m_portalsMutex;

    void clearPortalItems();
    void createPortalItems();
    QPointF gridToPixel(const QPointF& gridPos) const;
    QPointF pixelToGrid(const QPointF& pixelPos) const;
    bool isValidGridPosition(const QPointF& gridPos) const;
    bool lineIntersectsClosedPortal(const QPointF& start, const QPointF& end, const Portal* portal) const;
};

#endif // PORTALSYSTEM_H