#ifndef ATMOSPHERETOOLBOXWIDGET_H
#define ATMOSPHERETOOLBOXWIDGET_H

#include <QDockWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QButtonGroup>
#include <QTimer>
#include "controllers/AtmosphereState.h"

class AtmosphereManager;
class AmbientPlayer;
class MusicRemote;
class ToolSection;
class ColorPickerButton;

/**
 * @brief Dock widget providing full control over atmosphere effects.
 *
 * Provides UI controls for:
 * - Preset selection and management
 * - Lighting (intensity, ambient, exposure, tint, time of day)
 * - Weather (type, intensity, wind)
 * - Fog/Mist (density, height, color, animation, twist)
 * - Lightning (intensity, frequency)
 */
class AtmosphereToolboxWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit AtmosphereToolboxWidget(QWidget* parent = nullptr);
    ~AtmosphereToolboxWidget();

    void setAtmosphereManager(AtmosphereManager* manager);
    void setAudioSystems(AmbientPlayer* ambient, MusicRemote* remote);

public slots:
    // Update controls from external state change
    void updateFromState(const AtmosphereState& state);
    void onPresetChanged(const QString& presetName);

signals:
    void savePresetRequested();

private slots:
    // Preset controls
    void onPresetComboChanged(int index);
    void onTimeOfDayClicked(int id);
    void onRevertClicked();

    // Lighting controls
    void onLightingIntensityChanged(int value);
    void onAmbientLevelChanged(int value);
    void onExposureChanged(int value);
    void onLightingTintChanged(const QColor& color);

    // DM-only brightness/contrast (not applied to Player view)
    void onBrightnessChanged(int value);
    void onContrastChanged(int value);

    // Weather controls
    void onWeatherTypeChanged(int index);
    void onWeatherIntensityChanged(int value);
    void onWindStrengthChanged(int value);

    // Fog controls
    void onFogEnabledChanged(bool enabled);
    void onFogDensityChanged(int value);
    void onFogHeightChanged(int value);
    void onFogColorChanged(const QColor& color);
    void onFogAnimationChanged(int value);
    void onFogTwistChanged(int value);

    // Lightning controls
    void onLightningEnabledChanged(bool enabled);
    void onLightningIntensityChanged(int value);
    void onLightningFrequencyChanged(int value);

    // Audio controls
    void onAmbientVolumeChanged(int value);
    void onAmbientBrowseClicked();
    void onAmbientStopClicked();
    void onMusicPlayPauseClicked();
    void onMusicNextClicked();
    void onMusicPrevClicked();
    void onMusicVolumeChanged(int value);
    void onNowPlayingChanged();

private:
    void setupUI();
    void createPresetsSection();
    void createLightingSection();
    void createWeatherSection();
    void createFogSection();
    void createLightningSection();
    void createAudioSection();
    void applyDarkTheme();
    void populatePresetCombo();
    void connectSignals();

    // Build state from current control values
    AtmosphereState buildStateFromControls() const;

    // Apply state to effects (called when controls change)
    void applyCurrentState();

    // Schedule a debounced state update (for slider changes)
    void scheduleStateUpdate();

    // Update modified indicator
    void updateModifiedState();

    // Create a labeled slider row
    QWidget* createSliderRow(const QString& labelText, QSlider* slider, QLabel* valueLabel);

    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_mainLayout;

    // Sections
    ToolSection* m_presetsSection;
    ToolSection* m_lightingSection;
    ToolSection* m_weatherSection;
    ToolSection* m_fogSection;
    ToolSection* m_lightningSection;

    // Preset controls
    QComboBox* m_presetCombo;
    QPushButton* m_revertButton;
    QPushButton* m_savePresetButton;
    QButtonGroup* m_timeOfDayGroup;
    QPushButton* m_dawnButton;
    QPushButton* m_dayButton;
    QPushButton* m_duskButton;
    QPushButton* m_nightButton;
    QLabel* m_modifiedLabel;

    // Lighting controls
    QSlider* m_lightingIntensitySlider;
    QLabel* m_lightingIntensityValue;
    QSlider* m_ambientSlider;
    QLabel* m_ambientValue;
    QSlider* m_exposureSlider;
    QLabel* m_exposureValue;
    ColorPickerButton* m_lightingTintButton;

    // DM-only brightness/contrast
    QSlider* m_brightnessSlider;
    QLabel* m_brightnessValue;
    QSlider* m_contrastSlider;
    QLabel* m_contrastValue;

    // Weather controls
    QComboBox* m_weatherTypeCombo;
    QSlider* m_weatherIntensitySlider;
    QLabel* m_weatherIntensityValue;
    QSlider* m_windStrengthSlider;
    QLabel* m_windStrengthValue;

    // Fog controls
    QCheckBox* m_fogEnabledCheckbox;
    QSlider* m_fogDensitySlider;
    QLabel* m_fogDensityValue;
    QSlider* m_fogHeightSlider;
    QLabel* m_fogHeightValue;
    ColorPickerButton* m_fogColorButton;
    QSlider* m_fogAnimationSlider;
    QLabel* m_fogAnimationValue;
    QSlider* m_fogTwistSlider;
    QLabel* m_fogTwistValue;

    // Lightning controls
    QCheckBox* m_lightningEnabledCheckbox;
    QSlider* m_lightningIntensitySlider;
    QLabel* m_lightningIntensityValue;
    QSlider* m_lightningFrequencySlider;
    QLabel* m_lightningFrequencyValue;

    // Audio section
    ToolSection* m_audioSection;

    // Ambient controls
    QLabel* m_ambientTrackLabel;
    QSlider* m_ambientVolumeSlider;
    QLabel* m_ambientVolumeValue;
    QPushButton* m_ambientBrowseButton;
    QPushButton* m_ambientStopButton;

    // Music remote controls
    QLabel* m_nowPlayingLabel;
    QLabel* m_nowPlayingArtist;
    QPushButton* m_musicPrevButton;
    QPushButton* m_musicPlayPauseButton;
    QPushButton* m_musicNextButton;
    QSlider* m_musicVolumeSlider;
    QLabel* m_musicVolumeValue;

    // Audio references (not owned)
    AmbientPlayer* m_ambientPlayer;
    MusicRemote* m_musicRemote;

    // State tracking
    AtmosphereManager* m_atmosphereManager;
    QString m_currentPresetName;
    AtmosphereState m_presetBaseState;  // Original preset state for revert
    bool m_isModified;
    bool m_updatingFromState;  // Prevent feedback loops when updating controls

    // Fog animation parameters (not in AtmosphereState)
    qreal m_fogAnimationSpeed;
    qreal m_fogTwistAmount;

    // Debounce timer for slider changes (performance optimization)
    QTimer* m_debounceTimer;
    static constexpr int DEBOUNCE_MS = 150;  // Increased from 50ms for better performance

    // Flag to prevent feedback loop when we are the source of changes
    bool m_applyingState;
};

#endif // ATMOSPHERETOOLBOXWIDGET_H
