#include "graphics/PortalSystem.h"
#include "utils/DebugConsole.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QMutexLocker>
#include <QtMath>
#include <QGraphicsScene>

PortalSystem::PortalSystem()
    : QGraphicsItem()
    , m_mapSize(1000, 1000)
    , m_pixelsPerGrid(50)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setZValue(10);
}

PortalSystem::~PortalSystem()
{
    clearPortalItems();
}

void PortalSystem::setMapSize(const QSize& size)
{
    if (size.width() > 0 && size.height() > 0 &&
        size.width() <= 100000 && size.height() <= 100000) {
        m_mapSize = size;
        update();
    }
}

void PortalSystem::setPortals(const QList<PortalData>& portals)
{
    QMutexLocker locker(&m_portalsMutex);

    clearPortalItems();

    const int MAX_PORTALS = 1000;
    int portalCount = 0;

    for (const PortalData& portalData : portals) {
        if (portalCount >= MAX_PORTALS) {
            DebugConsole::warning(QString("Too many portals, limiting to %1").arg(MAX_PORTALS), "Portal");
            break;
        }

        if (!isValidGridPosition(portalData.position) ||
            !isValidGridPosition(portalData.bound1) ||
            !isValidGridPosition(portalData.bound2)) {
            DebugConsole::warning("Invalid portal position, skipping", "Portal");
            continue;
        }

        QPointF bound1 = portalData.bound1;
        QPointF bound2 = portalData.bound2;
        qreal distance = QLineF(bound1, bound2).length();

        if (distance < 0.001) {
            DebugConsole::warning("Portal bounds too close, skipping", "Portal");
            continue;
        }

        Portal* portal = new Portal(portalData.position, bound1, bound2,
                                   portalData.rotation, portalData.closed, portalData.freestanding);
        portal->setParentItem(this);
        portal->setPixelsPerGrid(m_pixelsPerGrid);

        m_portals.append(portal);
        portalCount++;
    }

    DebugConsole::info(QString("Created %1 portals").arg(m_portals.size()), "Portal");
    update();
}

void PortalSystem::clearPortals()
{
    QMutexLocker locker(&m_portalsMutex);
    clearPortalItems();
    update();
}

Portal* PortalSystem::findNearestPortal(const QPointF& point, qreal maxDistance) const
{
    QMutexLocker locker(&m_portalsMutex);

    Portal* nearest = nullptr;
    qreal minDistance = maxDistance;

    QPointF gridPoint = pixelToGrid(point);

    for (Portal* portal : m_portals) {
        if (!portal) continue;

        qreal distance = portal->distanceToPoint(gridPoint);
        if (distance < minDistance) {
            minDistance = distance;
            nearest = portal;
        }
    }

    return nearest;
}

bool PortalSystem::togglePortalAt(const QPointF& point, qreal maxDistance)
{
    Portal* portal = findNearestPortal(point, maxDistance);
    if (portal) {
        portal->toggleState();
        DebugConsole::info(QString("Toggled portal at (%1, %2) new state: %3")
                          .arg(portal->getPosition().x())
                          .arg(portal->getPosition().y())
                          .arg(portal->isClosed() ? "closed" : "open"), "Portal");
        return true;
    }
    return false;
}

void PortalSystem::setAllPortalsOpen(bool open)
{
    QMutexLocker locker(&m_portalsMutex);

    for (Portal* portal : m_portals) {
        if (portal) {
            portal->setOpen(open);
        }
    }

    DebugConsole::info(QString("Set all portals to %1").arg(open ? "open" : "closed"), "Portal");
}

void PortalSystem::setPixelsPerGrid(int pixelsPerGrid)
{
    if (pixelsPerGrid > 0 && pixelsPerGrid <= 500) {
        m_pixelsPerGrid = pixelsPerGrid;

        QMutexLocker locker(&m_portalsMutex);
        for (Portal* portal : m_portals) {
            if (portal) {
                portal->setPixelsPerGrid(pixelsPerGrid);
            }
        }
        update();
    }
}

bool PortalSystem::isPortalBlocking(const QPointF& start, const QPointF& end) const
{
    QMutexLocker locker(&m_portalsMutex);

    for (const Portal* portal : m_portals) {
        if (portal && portal->isClosed()) {
            if (lineIntersectsClosedPortal(start, end, portal)) {
                return true;
            }
        }
    }

    return false;
}

QRectF PortalSystem::boundingRect() const
{
    return QRectF(0, 0, m_mapSize.width(), m_mapSize.height());
}

void PortalSystem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void PortalSystem::clearPortalItems()
{
    for (Portal* portal : m_portals) {
        if (portal && portal->scene()) {
            portal->scene()->removeItem(portal);
        }
        delete portal;
    }
    m_portals.clear();
}

void PortalSystem::createPortalItems()
{
}

QPointF PortalSystem::gridToPixel(const QPointF& gridPos) const
{
    return QPointF(gridPos.x() * m_pixelsPerGrid, gridPos.y() * m_pixelsPerGrid);
}

QPointF PortalSystem::pixelToGrid(const QPointF& pixelPos) const
{
    if (m_pixelsPerGrid <= 0) {
        return QPointF(0, 0);
    }
    return QPointF(pixelPos.x() / m_pixelsPerGrid, pixelPos.y() / m_pixelsPerGrid);
}

bool PortalSystem::isValidGridPosition(const QPointF& gridPos) const
{
    const qreal MAX_COORD = 10000.0;
    return qIsFinite(gridPos.x()) && qIsFinite(gridPos.y()) &&
           qAbs(gridPos.x()) <= MAX_COORD && qAbs(gridPos.y()) <= MAX_COORD;
}

bool PortalSystem::lineIntersectsClosedPortal(const QPointF& start, const QPointF& end, const Portal* portal) const
{
    if (!portal || !portal->isClosed()) {
        return false;
    }

    QPointF portalStart = portal->getBound1();
    QPointF portalEnd = portal->getBound2();

    QLineF sightLine(start, end);
    QLineF portalLine(portalStart, portalEnd);

    QPointF intersectionPoint;
    QLineF::IntersectionType intersectionType = sightLine.intersects(portalLine, &intersectionPoint);

    if (intersectionType == QLineF::BoundedIntersection) {
        return true;
    }

    return false;
}