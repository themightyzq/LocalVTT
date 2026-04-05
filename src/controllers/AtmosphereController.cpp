#include "AtmosphereController.h"
#include "AtmosphereManager.h"
#include "AtmospherePreset.h"
#include "ui/MainWindow.h"
#include "ui/dialogs/SavePresetDialog.h"
#include "graphics/MapDisplay.h"
#include "graphics/PointLightSystem.h"
#include "graphics/PointLight.h"
#include "utils/CustomPresetManager.h"
#include <QDebug>
#include <QMessageBox>

AtmosphereController::AtmosphereController(QObject* parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_atmosphereManager(new AtmosphereManager(this))
    , m_mapDisplay(nullptr)
    , m_atmosphereMenu(nullptr)
    , m_presetsSubMenu(nullptr)
    , m_timeOfDaySubMenu(nullptr)
    , m_lightsSubMenu(nullptr)
    , m_customPresetsSubMenu(nullptr)
    , m_deletePresetsSubMenu(nullptr)
    , m_presetActionGroup(nullptr)
    , m_timeOfDayActionGroup(nullptr)
    , m_placeLightAction(nullptr)
    , m_savePresetAction(nullptr)
{
    // Connect manager signals
    connect(m_atmosphereManager, &AtmosphereManager::transitionStarted,
            this, &AtmosphereController::onTransitionStarted);
    connect(m_atmosphereManager, &AtmosphereManager::transitionCompleted,
            this, &AtmosphereController::onTransitionCompleted);

    // Connect custom preset manager signals
    connect(&CustomPresetManager::instance(), &CustomPresetManager::presetsChanged,
            this, &AtmosphereController::onCustomPresetsChanged);
}

AtmosphereController::~AtmosphereController()
{
    // Qt parent-child handles cleanup
}

void AtmosphereController::attachToMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
}

void AtmosphereController::setMapDisplay(MapDisplay* display)
{
    m_mapDisplay = display;
    m_atmosphereManager->setMapDisplay(display);
}

QMenu* AtmosphereController::createAtmosphereMenu(QWidget* parent)
{
    m_atmosphereMenu = new QMenu("Atmosphere", parent);

    // Create Presets submenu
    createPresetActions(m_atmosphereMenu);

    m_atmosphereMenu->addSeparator();

    // Create Time of Day submenu
    createTimeOfDayActions(m_atmosphereMenu);

    m_atmosphereMenu->addSeparator();

    // Create Lights submenu
    createLightActions(m_atmosphereMenu);

    return m_atmosphereMenu;
}

void AtmosphereController::createPresetActions(QMenu* menu)
{
    m_presetsSubMenu = menu->addMenu("Presets");
    m_presetActionGroup = new QActionGroup(this);
    m_presetActionGroup->setExclusive(true);

    // Get Phase 1 presets (just lighting, no weather/fog yet)
    QList<AtmospherePreset> presets = AtmospherePreset::getPhase1Presets();

    for (const auto& preset : presets) {
        QAction* action = m_presetsSubMenu->addAction(preset.name);
        action->setCheckable(true);
        action->setData(preset.name);
        action->setToolTip(preset.description);
        m_presetActionGroup->addAction(action);

        // Default to "Peaceful Day" as checked
        if (preset.name == "Peaceful Day") {
            action->setChecked(true);
        }
    }

    // Add separator and weather presets (Phase 2)
    m_presetsSubMenu->addSeparator();

    // Weather presets
    QAction* lightRainAction = m_presetsSubMenu->addAction("Light Rain");
    lightRainAction->setCheckable(true);
    lightRainAction->setData("Light Rain");
    lightRainAction->setToolTip("Gentle overcast rain");
    m_presetActionGroup->addAction(lightRainAction);

    QAction* stormyAction = m_presetsSubMenu->addAction("Stormy Night");
    stormyAction->setCheckable(true);
    stormyAction->setData("Stormy Night");
    stormyAction->setToolTip("Dark and ominous with heavy rain and lightning");
    m_presetActionGroup->addAction(stormyAction);

    QAction* snowAction = m_presetsSubMenu->addAction("Winter Snow");
    snowAction->setCheckable(true);
    snowAction->setData("Winter Snow");
    snowAction->setToolTip("Cold winter day with falling snow");
    m_presetActionGroup->addAction(snowAction);

    m_presetsSubMenu->addSeparator();

    // Atmospheric presets (Phase 3)
    QAction* dungeonAction = m_presetsSubMenu->addAction("Dungeon Depths");
    dungeonAction->setCheckable(true);
    dungeonAction->setData("Dungeon Depths");
    dungeonAction->setToolTip("Dark underground with thick shadows");
    m_presetActionGroup->addAction(dungeonAction);

    QAction* mysticalAction = m_presetsSubMenu->addAction("Mystical Fog");
    mysticalAction->setCheckable(true);
    mysticalAction->setData("Mystical Fog");
    mysticalAction->setToolTip("Ethereal purple mist for magical locations");
    m_presetActionGroup->addAction(mysticalAction);

    // Connect action group
    connect(m_presetActionGroup, &QActionGroup::triggered,
            this, &AtmosphereController::onPresetTriggered);

    // Custom presets section
    m_presetsSubMenu->addSeparator();

    // Custom presets submenu (will be populated dynamically)
    m_customPresetsSubMenu = m_presetsSubMenu->addMenu("Custom Presets");
    rebuildPresetsMenu();

    m_presetsSubMenu->addSeparator();

    // Save current as preset
    m_savePresetAction = m_presetsSubMenu->addAction("Save Current as Preset...");
    m_savePresetAction->setToolTip("Save the current atmosphere settings as a custom preset");
    connect(m_savePresetAction, &QAction::triggered,
            this, &AtmosphereController::onSaveCurrentPreset);

    // Delete custom preset submenu
    m_deletePresetsSubMenu = m_presetsSubMenu->addMenu("Delete Custom Preset");
    m_deletePresetsSubMenu->setToolTip("Delete a saved custom preset");
}

void AtmosphereController::createTimeOfDayActions(QMenu* menu)
{
    m_timeOfDaySubMenu = menu->addMenu("Time of Day");
    m_timeOfDayActionGroup = new QActionGroup(this);
    m_timeOfDayActionGroup->setExclusive(true);

    QAction* dawnAction = m_timeOfDaySubMenu->addAction("Dawn");
    dawnAction->setCheckable(true);
    dawnAction->setData(0);

    QAction* dayAction = m_timeOfDaySubMenu->addAction("Day");
    dayAction->setCheckable(true);
    dayAction->setData(1);
    dayAction->setChecked(true);  // Default

    QAction* duskAction = m_timeOfDaySubMenu->addAction("Dusk");
    duskAction->setCheckable(true);
    duskAction->setData(2);

    QAction* nightAction = m_timeOfDaySubMenu->addAction("Night");
    nightAction->setCheckable(true);
    nightAction->setData(3);

    m_timeOfDayActionGroup->addAction(dawnAction);
    m_timeOfDayActionGroup->addAction(dayAction);
    m_timeOfDayActionGroup->addAction(duskAction);
    m_timeOfDayActionGroup->addAction(nightAction);

    connect(m_timeOfDayActionGroup, &QActionGroup::triggered,
            this, &AtmosphereController::onTimeOfDayTriggered);
}

void AtmosphereController::onPresetTriggered(QAction* action)
{
    QString presetName = action->data().toString();
    qDebug() << "AtmosphereController: Applying preset:" << presetName;
    m_atmosphereManager->applyPreset(presetName);
}

void AtmosphereController::onTimeOfDayTriggered(QAction* action)
{
    int timeOfDay = action->data().toInt();
    qDebug() << "AtmosphereController: Setting time of day:" << timeOfDay;
    m_atmosphereManager->setTimeOfDay(timeOfDay);
}

void AtmosphereController::onTransitionStarted(const QString& presetName)
{
    qDebug() << "AtmosphereController: Transition started to" << presetName;
    // Could show a progress indicator in the status bar here
}

void AtmosphereController::onTransitionCompleted(const QString& presetName)
{
    qDebug() << "AtmosphereController: Transition completed -" << presetName;
    updateMenuState();
}

void AtmosphereController::updateMenuState()
{
    // Update preset menu checkmarks based on current state
    QString currentPreset = m_atmosphereManager->getCurrentPresetName();

    for (QAction* action : m_presetActionGroup->actions()) {
        if (action->data().toString() == currentPreset) {
            action->setChecked(true);
            break;
        }
    }

    // Update time of day checkmarks
    int currentTimeOfDay = m_atmosphereManager->getCurrentState().timeOfDay;
    for (QAction* action : m_timeOfDayActionGroup->actions()) {
        if (action->data().toInt() == currentTimeOfDay) {
            action->setChecked(true);
            break;
        }
    }
}

// Quick preset slots for toolbar buttons (if needed)
void AtmosphereController::applyPeacefulDay()
{
    m_atmosphereManager->applyPreset("Peaceful Day");
}

void AtmosphereController::applyGoldenDawn()
{
    m_atmosphereManager->applyPreset("Golden Dawn");
}

void AtmosphereController::applyWarmDusk()
{
    m_atmosphereManager->applyPreset("Warm Dusk");
}

void AtmosphereController::applyMoonlitNight()
{
    m_atmosphereManager->applyPreset("Moonlit Night");
}

void AtmosphereController::setDawn()
{
    m_atmosphereManager->setTimeOfDay(0);
}

void AtmosphereController::setDay()
{
    m_atmosphereManager->setTimeOfDay(1);
}

void AtmosphereController::setDusk()
{
    m_atmosphereManager->setTimeOfDay(2);
}

void AtmosphereController::setNight()
{
    m_atmosphereManager->setTimeOfDay(3);
}

void AtmosphereController::createLightActions(QMenu* menu)
{
    m_lightsSubMenu = menu->addMenu("Lights");

    // Toggle light placement mode
    m_placeLightAction = m_lightsSubMenu->addAction("Place Light Mode");
    m_placeLightAction->setCheckable(true);
    m_placeLightAction->setToolTip("Click on the map to place a light source");
    connect(m_placeLightAction, &QAction::toggled,
            this, &AtmosphereController::onPointLightPlacementToggled);

    m_lightsSubMenu->addSeparator();

    // Add light preset submenu
    QMenu* presetsMenu = m_lightsSubMenu->addMenu("Add Light");

    QAction* torchAction = presetsMenu->addAction("Torch");
    torchAction->setData(static_cast<int>(LightPreset::Torch));
    torchAction->setToolTip("Warm flickering torch light");

    QAction* lanternAction = presetsMenu->addAction("Lantern");
    lanternAction->setData(static_cast<int>(LightPreset::Lantern));
    lanternAction->setToolTip("Steady warm lantern light");

    QAction* campfireAction = presetsMenu->addAction("Campfire");
    campfireAction->setData(static_cast<int>(LightPreset::Campfire));
    campfireAction->setToolTip("Large orange flickering campfire");

    QAction* magicalAction = presetsMenu->addAction("Magical");
    magicalAction->setData(static_cast<int>(LightPreset::Magical));
    magicalAction->setToolTip("Ethereal blue magical light");

    QAction* moonbeamAction = presetsMenu->addAction("Moonbeam");
    moonbeamAction->setData(static_cast<int>(LightPreset::Moonbeam));
    moonbeamAction->setToolTip("Soft pale moonlight");

    // Connect all preset actions
    connect(torchAction, &QAction::triggered, this, [this, torchAction]() {
        onLightPresetTriggered(torchAction);
    });
    connect(lanternAction, &QAction::triggered, this, [this, lanternAction]() {
        onLightPresetTriggered(lanternAction);
    });
    connect(campfireAction, &QAction::triggered, this, [this, campfireAction]() {
        onLightPresetTriggered(campfireAction);
    });
    connect(magicalAction, &QAction::triggered, this, [this, magicalAction]() {
        onLightPresetTriggered(magicalAction);
    });
    connect(moonbeamAction, &QAction::triggered, this, [this, moonbeamAction]() {
        onLightPresetTriggered(moonbeamAction);
    });

    m_lightsSubMenu->addSeparator();

    // Clear all lights
    QAction* clearLightsAction = m_lightsSubMenu->addAction("Clear All Lights");
    clearLightsAction->setToolTip("Remove all point lights from the map");
    connect(clearLightsAction, &QAction::triggered,
            this, &AtmosphereController::onClearAllLights);
}

void AtmosphereController::onLightPresetTriggered(QAction* action)
{
    if (!m_mapDisplay) {
        qDebug() << "AtmosphereController: No map display set for light placement";
        return;
    }

    LightPreset preset = static_cast<LightPreset>(action->data().toInt());
    qDebug() << "AtmosphereController: Adding light preset:" << action->text();

    // Add light at center of visible area
    QRectF visibleRect = m_mapDisplay->mapToScene(m_mapDisplay->viewport()->rect()).boundingRect();
    QPointF centerPos = visibleRect.center();

    if (auto* lights = m_mapDisplay->getPointLightSystem()) {
        lights->addLightAtPosition(centerPos, preset);
    }
}

void AtmosphereController::onPointLightPlacementToggled(bool enabled)
{
    if (!m_mapDisplay) {
        qDebug() << "AtmosphereController: No map display set for light placement mode";
        return;
    }

    qDebug() << "AtmosphereController: Light placement mode:" << (enabled ? "ON" : "OFF");
    m_mapDisplay->setPointLightPlacementMode(enabled);
}

void AtmosphereController::onClearAllLights()
{
    if (!m_mapDisplay) {
        qDebug() << "AtmosphereController: No map display set for clearing lights";
        return;
    }

    qDebug() << "AtmosphereController: Clearing all point lights";
    m_mapDisplay->clearAllPointLights();
}

void AtmosphereController::onSaveCurrentPreset()
{
    SavePresetDialog dialog(m_mainWindow);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.presetName();
        QString description = dialog.presetDescription();
        AtmosphereState currentState = m_atmosphereManager->getCurrentState();

        if (CustomPresetManager::instance().savePreset(name, description, currentState)) {
            qDebug() << "AtmosphereController: Saved custom preset:" << name;
        } else {
            qWarning() << "AtmosphereController: Failed to save preset:" << name;
            QMessageBox::warning(m_mainWindow, "Save Failed",
                                 "Could not save the preset. Check permissions on the presets directory.");
        }
    }
}

void AtmosphereController::onDeleteCustomPreset(QAction* action)
{
    QString presetName = action->data().toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        m_mainWindow,
        "Delete Preset",
        QString("Are you sure you want to delete the preset \"%1\"?").arg(presetName),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (CustomPresetManager::instance().deletePreset(presetName)) {
            qDebug() << "AtmosphereController: Deleted custom preset:" << presetName;
        } else {
            qWarning() << "AtmosphereController: Failed to delete preset:" << presetName;
            QMessageBox::warning(m_mainWindow, "Delete Failed",
                                 "Could not delete the preset.");
        }
    }
}

void AtmosphereController::onCustomPresetsChanged()
{
    qDebug() << "AtmosphereController: Custom presets changed, rebuilding menu";
    rebuildPresetsMenu();
}

void AtmosphereController::rebuildPresetsMenu()
{
    if (!m_customPresetsSubMenu || !m_deletePresetsSubMenu) {
        return;
    }

    // Clear existing custom preset actions
    m_customPresetsSubMenu->clear();
    m_deletePresetsSubMenu->clear();

    // Get custom presets
    QList<AtmospherePreset> customPresets = CustomPresetManager::instance().getCustomPresets();

    if (customPresets.isEmpty()) {
        QAction* emptyAction = m_customPresetsSubMenu->addAction("(No custom presets)");
        emptyAction->setEnabled(false);

        QAction* emptyDeleteAction = m_deletePresetsSubMenu->addAction("(No custom presets)");
        emptyDeleteAction->setEnabled(false);
    } else {
        // Add custom presets to the Custom Presets submenu
        for (const auto& preset : customPresets) {
            QAction* action = m_customPresetsSubMenu->addAction(preset.name);
            action->setCheckable(true);
            action->setData(preset.name);
            action->setToolTip(preset.description.isEmpty() ? "Custom preset" : preset.description);
            m_presetActionGroup->addAction(action);

            // Connect to the preset triggered handler
            connect(action, &QAction::triggered, this, [this, action]() {
                onPresetTriggered(action);
            });
        }

        // Add presets to the Delete submenu
        for (const auto& preset : customPresets) {
            QAction* deleteAction = m_deletePresetsSubMenu->addAction(preset.name);
            deleteAction->setData(preset.name);
            connect(deleteAction, &QAction::triggered, this, [this, deleteAction]() {
                onDeleteCustomPreset(deleteAction);
            });
        }
    }

    // Update delete submenu visibility
    m_deletePresetsSubMenu->setEnabled(!customPresets.isEmpty());
}
