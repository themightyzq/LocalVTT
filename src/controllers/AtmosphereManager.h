#ifndef ATMOSPHEREMANAGER_H
#define ATMOSPHEREMANAGER_H

#include <QObject>
#include <QTimer>
#include <QEasingCurve>
#include "AtmosphereState.h"
#include "AtmospherePreset.h"

class AmbientPlayer;
class MusicRemote;
class MapDisplay;

// Central orchestrator for atmosphere effects and transitions
// Manages smooth transitions between atmospheric states
class AtmosphereManager : public QObject
{
    Q_OBJECT

public:
    explicit AtmosphereManager(QObject* parent = nullptr);
    ~AtmosphereManager();

    // Connect to map display (and its lighting overlay)
    void setMapDisplay(MapDisplay* display);
    MapDisplay* getMapDisplay() const { return m_mapDisplay; }

    // Current state access
    AtmosphereState getCurrentState() const { return m_currentState; }
    QString getCurrentPresetName() const { return m_currentPresetName; }

    // Audio access
    AmbientPlayer* getAmbientPlayer() const { return m_ambientPlayer; }
    MusicRemote* getMusicRemote() const { return m_musicRemote; }

    // Transition control
    void setTransitionDuration(int milliseconds);
    int getTransitionDuration() const { return m_transitionDuration; }

    void setEasingCurve(QEasingCurve::Type curve);
    QEasingCurve::Type getEasingCurve() const { return m_easingCurve.type(); }

    // Check if currently transitioning
    bool isTransitioning() const { return m_isTransitioning; }

    // Get progress (0.0 to 1.0) of current transition
    qreal getTransitionProgress() const;

public slots:
    // Apply a preset (with smooth transition)
    void applyPreset(const QString& presetName);
    void applyPreset(const AtmospherePreset& preset);

    // Apply a custom state (with smooth transition)
    void applyState(const AtmosphereState& state);

    // Immediately set state (no transition)
    void setStateImmediate(const AtmosphereState& state);

    // Time of day shortcuts
    void setTimeOfDay(int timeOfDay);  // 0=Dawn, 1=Day, 2=Dusk, 3=Night

    // Cancel current transition
    void cancelTransition();

    // Pause/resume transitions
    void pauseTransition();
    void resumeTransition();

signals:
    // Emitted when a transition starts
    void transitionStarted(const QString& toPresetName);

    // Emitted on each frame during transition (for UI progress bars)
    void transitionProgress(qreal progress);  // 0.0 to 1.0

    // Emitted when transition completes
    void transitionCompleted(const QString& presetName);

    // Emitted when state changes (during or after transition)
    void stateChanged(const AtmosphereState& state);

    // Emitted when current preset name changes
    void presetChanged(const QString& presetName);

private slots:
    void onTransitionTick();

private:
    void applyStateToOverlay(const AtmosphereState& state);
    void startTransition(const AtmosphereState& targetState, const QString& presetName);
    QString resolveAmbientTrackPath(const QString& track) const;

    // Map display reference (we get LightingOverlay fresh each time via MapDisplay)
    MapDisplay* m_mapDisplay;

    // Audio
    AmbientPlayer* m_ambientPlayer;
    MusicRemote* m_musicRemote;

    // Current and target states
    AtmosphereState m_currentState;
    AtmosphereState m_startState;
    AtmosphereState m_targetState;
    QString m_currentPresetName;
    QString m_targetPresetName;

    // Transition control
    QTimer* m_transitionTimer;
    int m_transitionDuration;  // milliseconds
    qint64 m_transitionStartTime;
    qint64 m_transitionPausedTime;
    bool m_isTransitioning;
    bool m_isPaused;
    QEasingCurve m_easingCurve;

    // Timer interval for 60 FPS updates
    static constexpr int TICK_INTERVAL_MS = 16;

    // Default transition duration (3 seconds for dramatic effect)
    static constexpr int DEFAULT_TRANSITION_MS = 3000;
};

#endif // ATMOSPHEREMANAGER_H
