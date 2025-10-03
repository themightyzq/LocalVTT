#ifndef LIGHTINGCONTROLLER_H
#define LIGHTINGCONTROLLER_H

#include <QObject>
#include <QUuid>

class MainWindow;
class MapDisplay;
class LightingOverlay;
class QSlider;
class QLabel;
class QAction;
class QButtonGroup;

class LightingController : public QObject
{
    Q_OBJECT

public:
    explicit LightingController(QObject *parent = nullptr);

    void attachToMainWindow(MainWindow* mainWindow);
    void setMapDisplay(MapDisplay* display);

    void setLightingControls(QAction* lightingAction,
                            QSlider* intensitySlider, QLabel* intensityLabel,
                            QSlider* ambientSlider, QLabel* ambientLabel,
                            QSlider* exposureSlider, QLabel* exposureLabel);

    void setTimeOfDayActions(QAction* dawn, QAction* day,
                             QAction* dusk, QAction* night);


public slots:
    void toggleLighting();
    void setTimeOfDay(int timeOfDay);
    void onLightingIntensityChanged(int value);
    void onAmbientLightChanged(int value);
    void onExposureChanged(int value);


    void updateLightingControls();

signals:
    void requestStatus(const QString& message, int timeout = 2000);

private:
    void updateTimeOfDayActions(int timeOfDay);

    MainWindow* m_mainWindow;
    MapDisplay* m_mapDisplay;

    QAction* m_lightingAction;
    QSlider* m_lightingIntensitySlider;
    QLabel* m_lightingIntensityLabel;
    QSlider* m_ambientLightSlider;
    QLabel* m_ambientLightLabel;
    QSlider* m_exposureSlider;
    QLabel* m_exposureLabel;

    QAction* m_dawnAction;
    QAction* m_dayAction;
    QAction* m_duskAction;
    QAction* m_nightAction;

};

#endif // LIGHTINGCONTROLLER_H