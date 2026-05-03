#include "AtmosphereManager.h"
#include "audio/AmbientPlayer.h"
#include "audio/MusicRemote.h"
#include "graphics/LightingOverlay.h"
#include "graphics/WeatherEffect.h"
#include "graphics/FogMistEffect.h"
#include "graphics/LightningEffect.h"
#include "graphics/MapDisplay.h"
#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

AtmosphereManager::AtmosphereManager(QObject* parent)
    : QObject(parent)
    , m_mapDisplay(nullptr)
    , m_ambientPlayer(nullptr)
    , m_musicRemote(nullptr)
    , m_transitionTimer(new QTimer(this))
    , m_transitionDuration(DEFAULT_TRANSITION_MS)
    , m_transitionStartTime(0)
    , m_transitionPausedTime(0)
    , m_isTransitioning(false)
    , m_isPaused(false)
    , m_easingCurve(QEasingCurve::InOutCubic)
{
    // Setup transition timer
    m_transitionTimer->setInterval(TICK_INTERVAL_MS);
    connect(m_transitionTimer, &QTimer::timeout, this, &AtmosphereManager::onTransitionTick);

    // Audio is out of scope per README §"What Crit VTT Does NOT Do".
    // m_ambientPlayer / m_musicRemote remain nullptr; AtmosphereToolboxWidget
    // hides the audio panel and setAudioSystems() null-guards both pointers.

    // Initialize with default state (Peaceful Day)
    m_currentState = AtmospherePreset::getPresetByName("Peaceful Day").state;
    m_currentPresetName = "Peaceful Day";
}

AtmosphereManager::~AtmosphereManager()
{
    if (m_transitionTimer->isActive()) {
        m_transitionTimer->stop();
    }
}

void AtmosphereManager::setMapDisplay(MapDisplay* display)
{
    m_mapDisplay = display;
    if (m_mapDisplay) {
        // Apply current state to the new display
        // LightingOverlay is lazily created, so applyStateToOverlay will handle it
        applyStateToOverlay(m_currentState);
    }
}

void AtmosphereManager::setTransitionDuration(int milliseconds)
{
    m_transitionDuration = qBound(100, milliseconds, 30000);  // 100ms to 30s
}

void AtmosphereManager::setEasingCurve(QEasingCurve::Type curve)
{
    m_easingCurve.setType(curve);
}

qreal AtmosphereManager::getTransitionProgress() const
{
    if (!m_isTransitioning) {
        return 1.0;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_isPaused) {
        currentTime = m_transitionPausedTime;
    }

    qint64 elapsed = currentTime - m_transitionStartTime;
    return qBound(0.0, static_cast<qreal>(elapsed) / m_transitionDuration, 1.0);
}

void AtmosphereManager::applyPreset(const QString& presetName)
{
    AtmospherePreset preset = AtmospherePreset::getPresetByName(presetName);
    applyPreset(preset);
}

void AtmosphereManager::applyPreset(const AtmospherePreset& preset)
{
    startTransition(preset.state, preset.name);
}

void AtmosphereManager::applyState(const AtmosphereState& state)
{
    startTransition(state, "Custom");
}

void AtmosphereManager::setStateImmediate(const AtmosphereState& state)
{
    // Stop any ongoing transition
    cancelTransition();

    // Apply immediately
    m_currentState = state;
    m_currentPresetName = "Custom";

    applyStateToOverlay(state);

    emit stateChanged(state);
    emit presetChanged(m_currentPresetName);
}

void AtmosphereManager::setTimeOfDay(int timeOfDay)
{
    // Create a state based on current but with new time of day
    AtmosphereState newState = m_currentState;
    newState.timeOfDay = qBound(0, timeOfDay, 3);

    // Adjust lighting based on time of day
    switch (timeOfDay) {
        case 0:  // Dawn
            newState.lightingIntensity = 0.8;
            newState.lightingTint = QColor(255, 200, 150);
            newState.ambientLevel = 0.2;
            break;
        case 1:  // Day
            newState.lightingIntensity = 1.0;
            newState.lightingTint = Qt::white;
            newState.ambientLevel = 0.3;
            break;
        case 2:  // Dusk
            newState.lightingIntensity = 0.6;
            newState.lightingTint = QColor(255, 150, 100);
            newState.ambientLevel = 0.15;
            break;
        case 3:  // Night
            newState.lightingIntensity = 0.3;
            newState.lightingTint = QColor(150, 170, 220);
            newState.ambientLevel = 0.1;
            break;
    }

    QString presetName = QString("Time: %1")
        .arg(timeOfDay == 0 ? "Dawn" :
             timeOfDay == 1 ? "Day" :
             timeOfDay == 2 ? "Dusk" : "Night");

    startTransition(newState, presetName);
}

void AtmosphereManager::cancelTransition()
{
    if (m_isTransitioning) {
        m_transitionTimer->stop();
        m_isTransitioning = false;
        m_isPaused = false;

        // Keep current interpolated state
        emit transitionCompleted(m_currentPresetName);
    }
}

void AtmosphereManager::pauseTransition()
{
    if (m_isTransitioning && !m_isPaused) {
        m_isPaused = true;
        m_transitionPausedTime = QDateTime::currentMSecsSinceEpoch();
        m_transitionTimer->stop();
    }
}

void AtmosphereManager::resumeTransition()
{
    if (m_isTransitioning && m_isPaused) {
        // Adjust start time to account for pause duration
        qint64 pauseDuration = QDateTime::currentMSecsSinceEpoch() - m_transitionPausedTime;
        m_transitionStartTime += pauseDuration;

        m_isPaused = false;
        m_transitionTimer->start();
    }
}

void AtmosphereManager::startTransition(const AtmosphereState& targetState, const QString& presetName)
{
    // Cancel any existing transition
    if (m_isTransitioning) {
        m_transitionTimer->stop();
    }

    // Store transition parameters
    m_startState = m_currentState;
    m_targetState = targetState;
    m_targetPresetName = presetName;

    // Start timing
    m_transitionStartTime = QDateTime::currentMSecsSinceEpoch();
    m_isTransitioning = true;
    m_isPaused = false;

    emit transitionStarted(presetName);

    // Start the transition timer
    m_transitionTimer->start();

    qDebug() << "AtmosphereManager: Starting transition to" << presetName
             << "over" << m_transitionDuration << "ms";
}

void AtmosphereManager::onTransitionTick()
{
    if (!m_isTransitioning) {
        m_transitionTimer->stop();
        return;
    }

    // Calculate progress with easing
    qreal rawProgress = getTransitionProgress();
    qreal easedProgress = m_easingCurve.valueForProgress(rawProgress);

    // Interpolate state
    m_currentState = AtmosphereState::lerp(m_startState, m_targetState, easedProgress);

    // Apply to overlay
    applyStateToOverlay(m_currentState);

    // Emit progress signal
    emit transitionProgress(rawProgress);
    emit stateChanged(m_currentState);

    // Check if complete
    if (rawProgress >= 1.0) {
        m_transitionTimer->stop();
        m_isTransitioning = false;
        m_currentState = m_targetState;  // Ensure exact final state
        m_currentPresetName = m_targetPresetName;

        applyStateToOverlay(m_currentState);

        emit transitionCompleted(m_currentPresetName);
        emit presetChanged(m_currentPresetName);

        qDebug() << "AtmosphereManager: Transition complete -" << m_currentPresetName;
    }
}

void AtmosphereManager::applyStateToOverlay(const AtmosphereState& state)
{
    // Always get fresh pointer from MapDisplay (it lazily creates the overlay)
    if (!m_mapDisplay) {
        return;
    }

    LightingOverlay* overlay = m_mapDisplay->getLightingOverlay();
    if (!overlay) {
        return;
    }

    // Apply lighting properties
    overlay->setEnabled(true);

    // Set time of day (this sets base lighting parameters)
    TimeOfDay tod = static_cast<TimeOfDay>(qBound(0, state.timeOfDay, 3));
    overlay->setTimeOfDay(tod);

    // Override with custom values for smooth transitions
    overlay->setLightingIntensity(state.lightingIntensity);
    overlay->setLightingTint(state.lightingTint);
    overlay->setAmbientLightLevel(state.ambientLevel);
    overlay->setExposure(state.exposure);

    // Phase 2: Apply weather effects
    WeatherEffect* weather = m_mapDisplay->getWeatherEffect();
    if (weather) {
        WeatherType weatherType = static_cast<WeatherType>(qBound(0, state.weatherType, 3));

        if (weatherType != WeatherType::None) {
            weather->setWeatherType(weatherType);
            weather->setIntensity(state.weatherIntensity);
            weather->setWindStrength(state.windStrength);
            weather->setEnabled(true);
        } else {
            weather->setEnabled(false);
        }
    }

    // Phase 3: Apply fog/mist effects
    FogMistEffect* fogMist = m_mapDisplay->getFogMistEffect();
    if (fogMist) {
        if (state.fogEnabled && state.fogDensity > 0.0) {
            fogMist->setDensity(state.fogDensity);
            fogMist->setHeight(state.fogHeight);
            fogMist->setColor(state.fogColor);

            // Auto-load fog texture if not already loaded
            if (!fogMist->hasTexture()) {
                // Try to load from resources/textures/fog/ (copied during build)
                QString texturePath = QCoreApplication::applicationDirPath() +
                                     "/../Resources/resources/textures/fog/texture_fog_01.png";
                if (QFile::exists(texturePath)) {
                    fogMist->loadFogTexture(texturePath);
                    fogMist->setTextureScale(1.5);  // Slightly larger tiles
                    fogMist->setTextureTwist(0.3);  // Subtle rotation
                } else {
                    qDebug() << "FogMistEffect: Texture not found at" << texturePath;
                }
            }

            fogMist->setEnabled(true);
        } else {
            fogMist->setEnabled(false);
        }
    }

    // Phase 4: Apply lightning effects
    LightningEffect* lightning = m_mapDisplay->getLightningEffect();
    if (lightning) {
        if (state.lightningEnabled && state.lightningIntensity > 0.0) {
            lightning->setIntensity(state.lightningIntensity);
            lightning->setFrequency(state.lightningFrequency);
            lightning->setEnabled(true);
        } else {
            lightning->setEnabled(false);
        }
    }

    // Audio — crossfade ambient track on state change
    if (m_ambientPlayer) {
        if (!state.ambientTrack.isEmpty()) {
            QString trackPath = resolveAmbientTrackPath(state.ambientTrack);
            if (!trackPath.isEmpty()) {
                m_ambientPlayer->crossfadeTo(trackPath, m_transitionDuration);
            }
            m_ambientPlayer->setVolume(state.ambientVolume);
        } else {
            m_ambientPlayer->stop(m_transitionDuration);
        }
    }

    // Music URL — open in system music app on state change
    if (m_musicRemote && !state.musicURL.isEmpty()) {
        m_musicRemote->openMusicURL(state.musicURL);
    }
}

QString AtmosphereManager::resolveAmbientTrackPath(const QString& track) const
{
    if (track.isEmpty()) return QString();

    // If absolute path, use directly
    if (QFileInfo(track).isAbsolute()) {
        return QFileInfo::exists(track) ? track : QString();
    }

    // Search in standard locations
    QStringList searchPaths = {
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/audio/" + track,
        QCoreApplication::applicationDirPath() + "/../Resources/resources/audio/" + track,
        QCoreApplication::applicationDirPath() + "/resources/audio/" + track,
    };

    for (const QString& path : searchPaths) {
        if (QFileInfo::exists(path)) return path;
    }

    return QString();
}
