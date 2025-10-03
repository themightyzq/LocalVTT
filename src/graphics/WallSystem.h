#ifndef WALLSYSTEM_H
#define WALLSYSTEM_H

#include <QGraphicsItem>
#include <QPointF>
#include <QLineF>
#include <QList>
#include <QMutex>
#include <QSize>

class PortalSystem;

class WallSystem : public QGraphicsItem
{
public:
    struct Wall {
        QLineF line;

        Wall() = default;
        Wall(const QPointF& start, const QPointF& end) : line(start, end) {}
        Wall(const QLineF& l) : line(l) {}
    };

    WallSystem();
    ~WallSystem();

    void setMapSize(const QSize& size);
    void setWalls(const QList<Wall>& walls);
    void clearWalls();

    bool isPointVisible(const QPointF& observer, const QPointF& target) const;
    QList<QPointF> getVisibleArea(const QPointF& observer, qreal maxDistance = 1000.0) const;

    void setDebugRenderingEnabled(bool enabled);
    bool isDebugRenderingEnabled() const { return m_debugRenderingEnabled; }

    void setPixelsPerGrid(int pixelsPerGrid) { m_pixelsPerGrid = pixelsPerGrid; }
    int getPixelsPerGrid() const { return m_pixelsPerGrid; }

    const QList<Wall>& getWalls() const { return m_walls; }

    // Portal system integration for line-of-sight blocking
    void setPortalSystem(PortalSystem* portalSystem) { m_portalSystem = portalSystem; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QSize m_mapSize;
    QList<Wall> m_walls;
    bool m_debugRenderingEnabled;
    int m_pixelsPerGrid;
    PortalSystem* m_portalSystem;  // For checking closed portal blocking

    mutable QMutex m_wallsMutex;

    bool lineIntersectsWall(const QLineF& line, const Wall& wall) const;
    QPointF gridToPixel(const QPointF& gridPos) const;
    QPointF pixelToGrid(const QPointF& pixelPos) const;
    bool isValidGridPosition(const QPointF& gridPos) const;
    QList<QPointF> castRaysForVisibility(const QPointF& observer, qreal maxDistance, int rayCount = 360) const;
};

#endif // WALLSYSTEM_H