#include "controllers/LightingController.h"
#include "ui/MainWindow.h"
#include "graphics/MapDisplay.h"
#include "graphics/LightingOverlay.h"
#include "utils/DebugConsole.h"
#include <QSlider>
#include <QLabel>
#include <QAction>
#include <QButtonGroup>

LightingController::LightingController(QObject *parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_mapDisplay(nullptr)
    , m_lightingAction(nullptr)
    , m_lightingIntensitySlider(nullptr)
    , m_lightingIntensityLabel(nullptr)
    , m_ambientLightSlider(nullptr)
    , m_ambientLightLabel(nullptr)
    , m_exposureSlider(nullptr)
    , m_exposureLabel(nullptr)
    , m_dawnAction(nullptr)
    , m_dayAction(nullptr)
    , m_duskAction(nullptr)
    , m_nightAction(nullptr)
{
}

void LightingController::attachToMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
}

void LightingController::setMapDisplay(MapDisplay* display)
{
    m_mapDisplay = display;
}

void LightingController::setLightingControls(QAction* lightingAction,
                                            QSlider* intensitySlider, QLabel* intensityLabel,
                                            QSlider* ambientSlider, QLabel* ambientLabel,
                                            QSlider* exposureSlider, QLabel* exposureLabel)
{
    m_lightingAction = lightingAction;
    m_lightingIntensitySlider = intensitySlider;
    m_lightingIntensityLabel = intensityLabel;
    m_ambientLightSlider = ambientSlider;
    m_ambientLightLabel = ambientLabel;
    m_exposureSlider = exposureSlider;
    m_exposureLabel = exposureLabel;

    if (m_lightingIntensitySlider) {
        connect(m_lightingIntensitySlider, &QSlider::valueChanged,
                this, &LightingController::onLightingIntensityChanged);
    }
    if (m_ambientLightSlider) {
        connect(m_ambientLightSlider, &QSlider::valueChanged,
                this, &LightingController::onAmbientLightChanged);
    }
    if (m_exposureSlider) {
        connect(m_exposureSlider, &QSlider::valueChanged,
                this, &LightingController::onExposureChanged);
    }
}

void LightingController::setTimeOfDayActions(QAction* dawn, QAction* day,
                                            QAction* dusk, QAction* night)
{
    m_dawnAction = dawn;
    m_dayAction = day;
    m_duskAction = dusk;
    m_nightAction = night;
}

void LightingController::toggleLighting()
{
    if (!m_mapDisplay) return;

    LightingOverlay* lightingOverlay = m_mapDisplay->getLightingOverlay();
    if (!lightingOverlay) return;

    bool enabled = !lightingOverlay->isEnabled();
    lightingOverlay->setEnabled(enabled);

    if (m_lightingAction) {
        m_lightingAction->setChecked(enabled);
    }

    updateLightingControls();
    m_mapDisplay->update();

    emit requestStatus(enabled ? "Lighting enabled" : "Lighting disabled");

    DebugConsole::info(
        enabled ? "Lighting system enabled" : "Lighting system disabled",
        "Lighting"
    );
}

void LightingController::setTimeOfDay(int timeOfDay)
{
    if (!m_mapDisplay) return;

    LightingOverlay* lightingOverlay = m_mapDisplay->getLightingOverlay();
    if (!lightingOverlay) return;

    lightingOverlay->setTimeOfDay(static_cast<TimeOfDay>(timeOfDay));
    updateTimeOfDayActions(timeOfDay);
    m_mapDisplay->update();

    QStringList times = {"Dawn", "Day", "Dusk", "Night"};
    emit requestStatus(QString("Time of day set to %1").arg(times[timeOfDay]));

    DebugConsole::info(
        QString("Time of day changed to %1").arg(times[timeOfDay]),
        "Lighting"
    );
}

void LightingController::onLightingIntensityChanged(int value)
{
    if (!m_mapDisplay) return;

    LightingOverlay* lightingOverlay = m_mapDisplay->getLightingOverlay();
    if (!lightingOverlay) return;

    float intensity = value / 100.0f;
    lightingOverlay->setLightingIntensity(intensity);

    if (m_lightingIntensityLabel) {
        m_lightingIntensityLabel->setText(QString("Intensity: %1%").arg(value));
    }

    m_mapDisplay->update();

    DebugConsole::info(
        QString("Lighting intensity set to %1%").arg(value),
        "Lighting"
    );
}

void LightingController::onAmbientLightChanged(int value)
{
    if (!m_mapDisplay) return;

    LightingOverlay* lightingOverlay = m_mapDisplay->getLightingOverlay();
    if (!lightingOverlay) return;

    float ambient = value / 100.0f;
    lightingOverlay->setAmbientLightLevel(ambient);

    if (m_ambientLightLabel) {
        m_ambientLightLabel->setText(QString("Ambient: %1%").arg(value));
    }

    m_mapDisplay->update();

    DebugConsole::info(
        QString("Ambient light set to %1%").arg(value),
        "Lighting"
    );
}

void LightingController::onExposureChanged(int value)
{
    if (!m_mapDisplay) return;

    LightingOverlay* lightingOverlay = m_mapDisplay->getLightingOverlay();
    if (!lightingOverlay) return;

    float exposure = value / 100.0f;
    lightingOverlay->setExposure(exposure);

    if (m_exposureLabel) {
        m_exposureLabel->setText(QString("Exposure: %1").arg(exposure, 0, 'f', 2));
    }

    m_mapDisplay->update();

    DebugConsole::info(
        QString("HDR exposure set to %1").arg(exposure, 0, 'f', 2),
        "Lighting"
    );
}

void LightingController::updateLightingControls()
{
    if (!m_mapDisplay) return;

    LightingOverlay* lightingOverlay = m_mapDisplay->getLightingOverlay();
    if (!lightingOverlay) return;

    bool enabled = lightingOverlay->isEnabled();

    if (m_lightingAction) {
        m_lightingAction->setChecked(enabled);
    }

    bool controlsEnabled = enabled;
    if (m_lightingIntensitySlider) m_lightingIntensitySlider->setEnabled(controlsEnabled);
    if (m_lightingIntensityLabel) m_lightingIntensityLabel->setEnabled(controlsEnabled);
    if (m_ambientLightSlider) m_ambientLightSlider->setEnabled(controlsEnabled);
    if (m_ambientLightLabel) m_ambientLightLabel->setEnabled(controlsEnabled);
    if (m_exposureSlider) m_exposureSlider->setEnabled(controlsEnabled);
    if (m_exposureLabel) m_exposureLabel->setEnabled(controlsEnabled);

    if (m_dawnAction) m_dawnAction->setEnabled(controlsEnabled);
    if (m_dayAction) m_dayAction->setEnabled(controlsEnabled);
    if (m_duskAction) m_duskAction->setEnabled(controlsEnabled);
    if (m_nightAction) m_nightAction->setEnabled(controlsEnabled);

    if (controlsEnabled) {
        int timeOfDay = static_cast<int>(lightingOverlay->getTimeOfDay());
        updateTimeOfDayActions(timeOfDay);

        if (m_lightingIntensitySlider) {
            int intensity = static_cast<int>(lightingOverlay->getLightingIntensity() * 100);
            m_lightingIntensitySlider->setValue(intensity);
        }

        if (m_ambientLightSlider) {
            int ambient = static_cast<int>(lightingOverlay->getAmbientLightLevel() * 100);
            m_ambientLightSlider->setValue(ambient);
        }

        if (m_exposureSlider) {
            int exposure = static_cast<int>(lightingOverlay->getExposure() * 100);
            m_exposureSlider->setValue(exposure);
        }
    }
}

void LightingController::updateTimeOfDayActions(int timeOfDay)
{
    if (m_dawnAction) m_dawnAction->setChecked(timeOfDay == 0);
    if (m_dayAction) m_dayAction->setChecked(timeOfDay == 1);
    if (m_duskAction) m_duskAction->setChecked(timeOfDay == 2);
    if (m_nightAction) m_nightAction->setChecked(timeOfDay == 3);
}