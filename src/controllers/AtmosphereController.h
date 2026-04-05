#ifndef ATMOSPHERECONTROLLER_H
#define ATMOSPHERECONTROLLER_H

#include <QObject>
#include <QMenu>
#include <QActionGroup>

class MainWindow;
class AtmosphereManager;
class MapDisplay;
class PointLightSystem;

// UI controller for atmosphere system
// Creates menu items and handles user interactions
class AtmosphereController : public QObject
{
    Q_OBJECT

public:
    explicit AtmosphereController(QObject* parent = nullptr);
    ~AtmosphereController();

    // Connect to main window
    void attachToMainWindow(MainWindow* mainWindow);

    // Set the map display to control
    void setMapDisplay(MapDisplay* display);

    // Get the atmosphere manager
    AtmosphereManager* getAtmosphereManager() const { return m_atmosphereManager; }

    // Create the Atmosphere menu (call from MainWindow)
    QMenu* createAtmosphereMenu(QWidget* parent);

public slots:
    // Quick preset application (for toolbar buttons if needed)
    void applyPeacefulDay();
    void applyGoldenDawn();
    void applyWarmDusk();
    void applyMoonlitNight();

    // Time of day shortcuts
    void setDawn();
    void setDay();
    void setDusk();
    void setNight();

private slots:
    void onPresetTriggered(QAction* action);
    void onTimeOfDayTriggered(QAction* action);
    void onTransitionStarted(const QString& presetName);
    void onTransitionCompleted(const QString& presetName);
    void onLightPresetTriggered(QAction* action);
    void onPointLightPlacementToggled(bool enabled);
    void onClearAllLights();
    // Custom presets
    void onSaveCurrentPreset();
    void onDeleteCustomPreset(QAction* action);
    void onCustomPresetsChanged();

private:
    void createPresetActions(QMenu* menu);
    void createTimeOfDayActions(QMenu* menu);
    void createLightActions(QMenu* menu);
    void updateMenuState();
    void rebuildPresetsMenu();

    MainWindow* m_mainWindow;
    AtmosphereManager* m_atmosphereManager;
    MapDisplay* m_mapDisplay;

    // Menu and actions
    QMenu* m_atmosphereMenu;
    QMenu* m_presetsSubMenu;
    QMenu* m_timeOfDaySubMenu;
    QMenu* m_lightsSubMenu;
    QMenu* m_customPresetsSubMenu;
    QMenu* m_deletePresetsSubMenu;
    QActionGroup* m_presetActionGroup;
    QActionGroup* m_timeOfDayActionGroup;
    QAction* m_placeLightAction;
    QAction* m_savePresetAction;
};

#endif // ATMOSPHERECONTROLLER_H
