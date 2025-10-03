#ifndef ZOOMMANAGER_H
#define ZOOMMANAGER_H

#include <QObject>
#include <QPointF>

class QGraphicsView;
class QPropertyAnimation;
class QTimer;
class ZoomIndicator;

class ZoomManager : public QObject
{
    Q_OBJECT

public:
    explicit ZoomManager(QGraphicsView* view, QObject* parent = nullptr);
    ~ZoomManager();

    void setZoomLevel(qreal zoomLevel);
    void zoomToPreset(qreal zoomLevel);
    void fitToView();
    void animateZoomTo(qreal targetZoom, const QPointF& centerPoint = QPointF());

    void startSmoothPan(const QPointF& velocity);
    void stopSmoothPan();

    qreal getZoomLevel() const { return m_zoomFactor; }
    qreal getTargetZoomLevel() const { return m_targetZoomFactor; }

    void setZoomControlsEnabled(bool enabled) { m_zoomControlsEnabled = enabled; }
    bool isZoomControlsEnabled() const { return m_zoomControlsEnabled; }

    void handleWheelEvent(int angleDelta, const QPointF& cursorPos);
    void syncZoomLevel(qreal zoomLevel, const QPointF& centerPoint = QPointF());

    static constexpr qreal MIN_ZOOM = 0.1;
    static constexpr qreal MAX_ZOOM = 5.0;

signals:
    void zoomChanged(qreal zoomLevel);

private slots:
    void animateZoom();
    void updateSmoothPan();
    void finishZoomAccumulation();

private:
    void applyZoom(qreal zoomLevel);
    bool shouldDisableAnimation() const;

    QGraphicsView* m_view;
    ZoomIndicator* m_zoomIndicator;

    qreal m_zoomFactor;
    qreal m_targetZoomFactor;
    bool m_zoomControlsEnabled;

    QPropertyAnimation* m_zoomAnimation;
    QTimer* m_smoothPanTimer;
    QTimer* m_zoomAccumulationTimer;

    qreal m_animationStartZoom;
    qreal m_animationTargetZoom;
    QPointF m_zoomAnimationCenter;
    bool m_isZoomAnimating;

    QPointF m_zoomCursorPos;
    QPointF m_zoomScenePos;
    bool m_zoomCenterOnCursor;

    QPointF m_panVelocity;
};

#endif // ZOOMMANAGER_H