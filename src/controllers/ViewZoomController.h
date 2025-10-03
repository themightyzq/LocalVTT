#ifndef VIEWZOOMCONTROLLER_H
#define VIEWZOOMCONTROLLER_H

#include <QObject>

class MapDisplay;
class PlayerWindow;
class QAction;
class QLabel;

class ViewZoomController : public QObject
{
    Q_OBJECT

public:
    explicit ViewZoomController(QObject *parent = nullptr);

    void setMapDisplay(MapDisplay* display);
    void setPlayerWindow(PlayerWindow* playerWindow);
    void setZoomActions(QAction* fitAction, QAction* zoomInAction, QAction* zoomOutAction);
    void setZoomStatusLabel(QLabel* label);

    qreal getCurrentZoom() const;

public slots:
    void fitToScreen();
    void zoomIn();
    void zoomOut();
    void updateZoomStatus();
    void syncZoomWithPlayer(qreal zoomLevel);
    void togglePlayerViewMode();

signals:
    void zoomChanged(qreal zoomLevel);
    void requestStatus(const QString& message, int timeout = 2000);

private:
    MapDisplay* m_mapDisplay;
    PlayerWindow* m_playerWindow;
    QAction* m_fitToScreenAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QLabel* m_zoomStatusLabel;
    bool m_playerViewModeEnabled;
};

#endif // VIEWZOOMCONTROLLER_H