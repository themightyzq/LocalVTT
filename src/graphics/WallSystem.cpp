#include "graphics/WallSystem.h"
#include "graphics/PortalSystem.h"
#include "utils/DebugConsole.h"
#include <QPainter>
#include <QMutexLocker>
#include <QtMath>

WallSystem::WallSystem()
    : m_debugRenderingEnabled(false)
    , m_pixelsPerGrid(50)
    , m_portalSystem(nullptr)
{
    setZValue(5);
}

WallSystem::~WallSystem()
{
}

void WallSystem::setMapSize(const QSize& size)
{
    if (size.width() <= 0 || size.height() <= 0) {
        DebugConsole::warning(QString("Invalid map size: %1x%2").arg(size.width()).arg(size.height()), "Wall");
        return;
    }

    if (size.width() > 50000 || size.height() > 50000) {
        DebugConsole::warning(QString("Map size too large: %1x%2").arg(size.width()).arg(size.height()), "Wall");
        return;
    }

    m_mapSize = size;
    update();
}

void WallSystem::setWalls(const QList<Wall>& walls)
{
    QMutexLocker locker(&m_wallsMutex);

    const int MAX_WALLS = 10000;
    if (walls.size() > MAX_WALLS) {
        DebugConsole::warning(QString("Too many walls, limiting to %1").arg(MAX_WALLS), "Wall");
        m_walls = walls.mid(0, MAX_WALLS);
    } else {
        m_walls = walls;
    }

    update();
}

void WallSystem::clearWalls()
{
    QMutexLocker locker(&m_wallsMutex);
    m_walls.clear();
    update();
}

bool WallSystem::isPointVisible(const QPointF& observer, const QPointF& target) const
{
    QMutexLocker locker(&m_wallsMutex);

    if (!isValidGridPosition(observer) || !isValidGridPosition(target)) {
        return false;
    }

    QLineF sightLine(observer, target);

    // Check walls first
    for (const Wall& wall : m_walls) {
        if (lineIntersectsWall(sightLine, wall)) {
            return false;
        }
    }

    // Check closed portals if portal system is available
    if (m_portalSystem && m_portalSystem->isPortalBlocking(observer, target)) {
        return false;
    }

    return true;
}

QList<QPointF> WallSystem::getVisibleArea(const QPointF& observer, qreal maxDistance) const
{
    if (!isValidGridPosition(observer)) {
        return QList<QPointF>();
    }

    if (maxDistance <= 0 || maxDistance > 10000) {
        maxDistance = 1000;
    }

    return castRaysForVisibility(observer, maxDistance);
}

void WallSystem::setDebugRenderingEnabled(bool enabled)
{
    if (m_debugRenderingEnabled != enabled) {
        m_debugRenderingEnabled = enabled;
        update();
    }
}

QRectF WallSystem::boundingRect() const
{
    return QRectF(0, 0, m_mapSize.width(), m_mapSize.height());
}

void WallSystem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!m_debugRenderingEnabled) {
        return;
    }

    QMutexLocker locker(&m_wallsMutex);

    painter->setPen(QPen(QColor(255, 0, 0, 180), 2));

    for (const Wall& wall : m_walls) {
        QPointF start = gridToPixel(wall.line.p1());
        QPointF end = gridToPixel(wall.line.p2());

        if (start.x() >= 0 && start.x() < m_mapSize.width() &&
            start.y() >= 0 && start.y() < m_mapSize.height() &&
            end.x() >= 0 && end.x() < m_mapSize.width() &&
            end.y() >= 0 && end.y() < m_mapSize.height()) {

            painter->drawLine(start, end);
        }
    }
}

bool WallSystem::lineIntersectsWall(const QLineF& line, const Wall& wall) const
{
    QPointF intersectionPoint;
    QLineF::IntersectionType intersectionType = line.intersects(wall.line, &intersectionPoint);

    if (intersectionType != QLineF::BoundedIntersection) {
        return false;
    }

    qreal lineLength = line.length();
    if (lineLength < 0.001) {
        return false;
    }

    qreal distanceToIntersection = QLineF(line.p1(), intersectionPoint).length();

    return distanceToIntersection < lineLength - 0.001;
}

QPointF WallSystem::gridToPixel(const QPointF& gridPos) const
{
    if (m_pixelsPerGrid <= 0) {
        DebugConsole::warning(QString("Invalid pixels per grid: %1").arg(m_pixelsPerGrid), "Wall");
        return QPointF();
    }

    return QPointF(gridPos.x() * m_pixelsPerGrid, gridPos.y() * m_pixelsPerGrid);
}

QPointF WallSystem::pixelToGrid(const QPointF& pixelPos) const
{
    if (m_pixelsPerGrid <= 0) {
        DebugConsole::warning(QString("Invalid pixels per grid: %1").arg(m_pixelsPerGrid), "Wall");
        return QPointF();
    }

    return QPointF(pixelPos.x() / m_pixelsPerGrid, pixelPos.y() / m_pixelsPerGrid);
}

bool WallSystem::isValidGridPosition(const QPointF& gridPos) const
{
    if (m_mapSize.isEmpty() || m_pixelsPerGrid <= 0) {
        return false;
    }

    qreal maxGridX = m_mapSize.width() / static_cast<qreal>(m_pixelsPerGrid);
    qreal maxGridY = m_mapSize.height() / static_cast<qreal>(m_pixelsPerGrid);

    return gridPos.x() >= 0 && gridPos.x() <= maxGridX &&
           gridPos.y() >= 0 && gridPos.y() <= maxGridY;
}

QList<QPointF> WallSystem::castRaysForVisibility(const QPointF& observer, qreal maxDistance, int rayCount) const
{
    QList<QPointF> visiblePoints;

    if (rayCount <= 0 || rayCount > 3600) {
        rayCount = 360;
    }

    qreal angleStep = 360.0 / rayCount;

    for (int i = 0; i < rayCount; ++i) {
        qreal angle = i * angleStep;
        qreal radians = qDegreesToRadians(angle);

        QPointF direction(qCos(radians), qSin(radians));
        QPointF endPoint = observer + direction * maxDistance;

        QLineF ray(observer, endPoint);
        QPointF closestIntersection = endPoint;
        qreal closestDistance = maxDistance;

        // Check walls for intersection
        for (const Wall& wall : m_walls) {
            QPointF intersectionPoint;
            QLineF::IntersectionType intersectionType = ray.intersects(wall.line, &intersectionPoint);

            if (intersectionType == QLineF::BoundedIntersection) {
                qreal distance = QLineF(observer, intersectionPoint).length();
                if (distance < closestDistance) {
                    closestDistance = distance;
                    closestIntersection = intersectionPoint;
                }
            }
        }

        // Check portal system for blocking if available
        if (m_portalSystem && m_portalSystem->isPortalBlocking(observer, endPoint)) {
            // Find intersection with the closest closed portal
            // For now, we'll use the existing closest intersection
            // A more sophisticated implementation could find the exact portal intersection point
        }

        if (isValidGridPosition(closestIntersection)) {
            visiblePoints.append(closestIntersection);
        }
    }

    return visiblePoints;
}