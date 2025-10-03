#ifndef MOUSEINPUTMANAGER_H
#define MOUSEINPUTMANAGER_H

#include <QObject>
#include <QPointF>
#include <QPoint>
#include <QRectF>
#include <QList>
#include <QUuid>

class QMouseEvent;
class QKeyEvent;
class MapDisplay;
class FogOfWar;
class QGraphicsRectItem;

enum class FogToolMode;

class MouseInputManager : public QObject
{
    Q_OBJECT

public:
    explicit MouseInputManager(MapDisplay* mapDisplay, QObject* parent = nullptr);
    ~MouseInputManager() = default;

    void handleMousePress(QMouseEvent* event);
    void handleMouseMove(QMouseEvent* event);
    void handleMouseRelease(QMouseEvent* event);
    void handleMouseDoubleClick(QMouseEvent* event);
    void handleKeyPress(QKeyEvent* event);

    void setFogBrushSize(int size) { m_fogBrushSize = size; }
    int getFogBrushSize() const { return m_fogBrushSize; }

    void setFogHideModeEnabled(bool enabled) { m_fogHideModeEnabled = enabled; }
    bool isFogHideModeEnabled() const { return m_fogHideModeEnabled; }

    void setFogRectangleModeEnabled(bool enabled);
    bool isFogRectangleModeEnabled() const { return m_fogRectangleModeEnabled; }


    void setPointLightPlacementMode(bool enabled) { m_pointLightPlacementMode = enabled; }
    bool isPointLightPlacementMode() const { return m_pointLightPlacementMode; }

    bool isPanning() const { return m_isPanning; }
    QPointF getPanVelocity() const { return m_panVelocity; }


signals:
    void panStarted();
    void panMoved(const QPoint& delta);
    void panEnded();
    void zoomRequested(qreal factor, const QPointF& cursorPos);
    void pingRequested(const QPointF& scenePos);
    void gmBeaconRequested(const QPointF& scenePos);
    void pointLightRequested(const QPointF& scenePos);
    void pointLightDoubleClicked(const QUuid& lightId);
    void portalToggleRequested(const QPointF& scenePos);

public:
    bool isZoomControlsEnabled() const { return true; }

private:
    void calculateReleaseVelocity();
    void handleFogToolMousePress(QMouseEvent* event);
    void handleFogToolMouseMove(QMouseEvent* event);
    void handleFogToolMouseRelease(QMouseEvent* event);

    MapDisplay* m_mapDisplay;

    bool m_isPanning;
    QPoint m_lastPanPoint;
    QList<QPointF> m_velocityHistory;
    QList<qint64> m_velocityTimestamps;
    qint64 m_lastMoveTime;
    QPointF m_panVelocity;

    int m_fogBrushSize;
    bool m_fogHideModeEnabled;
    bool m_fogRectangleModeEnabled;

    bool m_isSelectingRectangle;
    QPointF m_rectangleStartPos;
    QRectF m_currentSelectionRect;
    QGraphicsRectItem* m_selectionRectIndicator;
    bool m_rectangleHideMode;  // Track if rectangle operation is in hide mode


    bool m_pointLightPlacementMode;
};

#endif // MOUSEINPUTMANAGER_H