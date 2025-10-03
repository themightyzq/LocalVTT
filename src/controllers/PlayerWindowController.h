#ifndef PLAYERWINDOWCONTROLLER_H
#define PLAYERWINDOWCONTROLLER_H

#include <QObject>

class MainWindow;
class PlayerWindow;
class MapDisplay;
class QAction;

class PlayerWindowController : public QObject
{
    Q_OBJECT

public:
    explicit PlayerWindowController(QObject *parent = nullptr);
    ~PlayerWindowController();

    void attachToMainWindow(MainWindow* mainWindow);
    void setMapDisplay(MapDisplay* display);
    void setToggleAction(QAction* action);

    PlayerWindow* getPlayerWindow() const { return m_playerWindow; }
    bool isPlayerWindowVisible() const;

public slots:
    void togglePlayerWindow();
    void autoOpenPlayerWindow();
    void updatePlayerWindow();
    void syncWithMainWindow();

signals:
    void playerWindowToggled(bool visible);
    void requestStatus(const QString& message, int timeout = 2000);

private:
    void createPlayerWindow();
    void positionPlayerWindow();
    void destroyPlayerWindow();

    MainWindow* m_mainWindow;
    MapDisplay* m_mapDisplay;
    PlayerWindow* m_playerWindow;
    QAction* m_toggleAction;
};

#endif // PLAYERWINDOWCONTROLLER_H