#include "AtmosphereToolboxWidget.h"
#include "ToolboxWidget.h"  // For ToolSection
#include "widgets/ColorPickerButton.h"
#include "controllers/AtmosphereManager.h"
#include "controllers/AtmospherePreset.h"
#include "graphics/MapDisplay.h"
#include "graphics/FogMistEffect.h"
#include "graphics/LightingOverlay.h"
#include "DarkTheme.h"

#include "audio/AmbientPlayer.h"
#include "audio/MusicRemote.h"
#include <QHBoxLayout>
#include <QToolButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFrame>

AtmosphereToolboxWidget::AtmosphereToolboxWidget(QWidget* parent)
    : QDockWidget(parent)
    , m_audioSection(nullptr)
    , m_ambientTrackLabel(nullptr)
    , m_ambientVolumeSlider(nullptr)
    , m_ambientVolumeValue(nullptr)
    , m_ambientBrowseButton(nullptr)
    , m_ambientStopButton(nullptr)
    , m_nowPlayingLabel(nullptr)
    , m_nowPlayingArtist(nullptr)
    , m_musicPrevButton(nullptr)
    , m_musicPlayPauseButton(nullptr)
    , m_musicNextButton(nullptr)
    , m_musicVolumeSlider(nullptr)
    , m_musicVolumeValue(nullptr)
    , m_ambientPlayer(nullptr)
    , m_musicRemote(nullptr)
    , m_atmosphereManager(nullptr)
    , m_isModified(false)
    , m_updatingFromState(false)
    , m_fogAnimationSpeed(0.5)
    , m_fogTwistAmount(0.3)
    , m_debounceTimer(nullptr)
    , m_applyingState(false)
{
    setWindowTitle(tr("Atmosphere"));
    setObjectName("AtmosphereToolbox");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setMinimumWidth(280);
    setMaximumWidth(400);

    // Create debounce timer for slider changes (performance optimization)
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(DEBOUNCE_MS);
    connect(m_debounceTimer, &QTimer::timeout, this, &AtmosphereToolboxWidget::applyCurrentState);

    setupUI();
    applyDarkTheme();
    populatePresetCombo();
    connectSignals();
}

AtmosphereToolboxWidget::~AtmosphereToolboxWidget()
{
}

void AtmosphereToolboxWidget::setAtmosphereManager(AtmosphereManager* manager)
{
    if (m_atmosphereManager) {
        disconnect(m_atmosphereManager, nullptr, this, nullptr);
    }

    m_atmosphereManager = manager;

    if (m_atmosphereManager) {
        connect(m_atmosphereManager, &AtmosphereManager::stateChanged,
                this, &AtmosphereToolboxWidget::updateFromState);
        connect(m_atmosphereManager, &AtmosphereManager::presetChanged,
                this, &AtmosphereToolboxWidget::onPresetChanged);

        // Initialize from current state
        updateFromState(m_atmosphereManager->getCurrentState());
        onPresetChanged(m_atmosphereManager->getCurrentPresetName());
    }
}

void AtmosphereToolboxWidget::setupUI()
{
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_contentWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);

    createPresetsSection();
    // Audio panel removed — feature is out of scope per README.
    createLightingSection();
    createWeatherSection();
    createFogSection();
    createLightningSection();

    m_mainLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    setWidget(m_scrollArea);
}

void AtmosphereToolboxWidget::createPresetsSection()
{
    m_presetsSection = new ToolSection(tr("Presets"), m_contentWidget);
    m_presetsSection->setCollapsible(false);

    // Preset combo box
    QWidget* comboRow = new QWidget();
    QHBoxLayout* comboLayout = new QHBoxLayout(comboRow);
    comboLayout->setContentsMargins(0, 0, 0, 0);
    comboLayout->setSpacing(4);

    m_presetCombo = new QComboBox();
    m_presetCombo->setMinimumWidth(150);
    m_presetCombo->setToolTip(tr("Select atmosphere preset"));
    comboLayout->addWidget(m_presetCombo, 1);

    m_modifiedLabel = new QLabel();
    m_modifiedLabel->setStyleSheet("color: #F39C12; font-size: 10px;");
    m_modifiedLabel->hide();
    comboLayout->addWidget(m_modifiedLabel);

    m_presetsSection->addWidget(comboRow);

    // Revert and Save buttons
    QWidget* buttonRow = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 4, 0, 0);
    buttonLayout->setSpacing(4);

    m_revertButton = new QPushButton(tr("Revert"));
    m_revertButton->setToolTip(tr("Revert to preset values"));
    m_revertButton->setEnabled(false);
    buttonLayout->addWidget(m_revertButton);

    m_savePresetButton = new QPushButton(tr("Save as Preset..."));
    m_savePresetButton->setToolTip(tr("Save current settings as a new preset"));
    buttonLayout->addWidget(m_savePresetButton);

    buttonLayout->addStretch();
    m_presetsSection->addWidget(buttonRow);

    // Time of Day buttons
    QWidget* todRow = new QWidget();
    QVBoxLayout* todLayout = new QVBoxLayout(todRow);
    todLayout->setContentsMargins(0, 8, 0, 0);
    todLayout->setSpacing(4);

    QLabel* todLabel = new QLabel(tr("Time of Day:"));
    todLabel->setStyleSheet("color: #A0A0A0; font-size: 11px;");
    todLayout->addWidget(todLabel);

    QWidget* todButtons = new QWidget();
    QHBoxLayout* todButtonLayout = new QHBoxLayout(todButtons);
    todButtonLayout->setContentsMargins(0, 0, 0, 0);
    todButtonLayout->setSpacing(4);

    m_timeOfDayGroup = new QButtonGroup(this);
    m_timeOfDayGroup->setExclusive(true);

    m_dawnButton = new QPushButton(tr("Dawn"));
    m_dawnButton->setCheckable(true);
    m_dawnButton->setToolTip(tr("Warm sunrise lighting"));
    m_timeOfDayGroup->addButton(m_dawnButton, 0);
    todButtonLayout->addWidget(m_dawnButton);

    m_dayButton = new QPushButton(tr("Day"));
    m_dayButton->setCheckable(true);
    m_dayButton->setChecked(true);
    m_dayButton->setToolTip(tr("Bright daylight"));
    m_timeOfDayGroup->addButton(m_dayButton, 1);
    todButtonLayout->addWidget(m_dayButton);

    m_duskButton = new QPushButton(tr("Dusk"));
    m_duskButton->setCheckable(true);
    m_duskButton->setToolTip(tr("Orange sunset lighting"));
    m_timeOfDayGroup->addButton(m_duskButton, 2);
    todButtonLayout->addWidget(m_duskButton);

    m_nightButton = new QPushButton(tr("Night"));
    m_nightButton->setCheckable(true);
    m_nightButton->setToolTip(tr("Cool moonlight"));
    m_timeOfDayGroup->addButton(m_nightButton, 3);
    todButtonLayout->addWidget(m_nightButton);

    todLayout->addWidget(todButtons);
    m_presetsSection->addWidget(todRow);

    m_mainLayout->addWidget(m_presetsSection);
}

void AtmosphereToolboxWidget::createLightingSection()
{
    m_lightingSection = new ToolSection(tr("Lighting"), m_contentWidget);
    m_lightingSection->setCollapsible(true);
    m_lightingSection->setExpanded(true);

    // Intensity slider
    m_lightingIntensitySlider = new QSlider(Qt::Horizontal);
    m_lightingIntensitySlider->setRange(0, 100);
    m_lightingIntensitySlider->setValue(100);
    m_lightingIntensitySlider->setToolTip(tr("Overall lighting brightness"));
    m_lightingIntensityValue = new QLabel("100%");
    m_lightingSection->addWidget(createSliderRow(tr("Intensity"), m_lightingIntensitySlider, m_lightingIntensityValue));

    // Ambient slider
    m_ambientSlider = new QSlider(Qt::Horizontal);
    m_ambientSlider->setRange(0, 100);
    m_ambientSlider->setValue(20);
    m_ambientSlider->setToolTip(tr("Ambient light level in shadows"));
    m_ambientValue = new QLabel("20%");
    m_lightingSection->addWidget(createSliderRow(tr("Ambient"), m_ambientSlider, m_ambientValue));

    // Exposure slider
    m_exposureSlider = new QSlider(Qt::Horizontal);
    m_exposureSlider->setRange(10, 300);  // 0.1 to 3.0
    m_exposureSlider->setValue(100);
    m_exposureSlider->setToolTip(tr("HDR exposure adjustment"));
    m_exposureValue = new QLabel("1.0");
    m_lightingSection->addWidget(createSliderRow(tr("Exposure"), m_exposureSlider, m_exposureValue));

    // Tint color
    QWidget* tintRow = new QWidget();
    QHBoxLayout* tintLayout = new QHBoxLayout(tintRow);
    tintLayout->setContentsMargins(0, 0, 0, 0);
    tintLayout->setSpacing(8);

    QLabel* tintLabel = new QLabel(tr("Tint"));
    tintLabel->setFixedWidth(70);
    tintLabel->setStyleSheet("color: #A0A0A0; font-size: 11px;");
    tintLayout->addWidget(tintLabel);

    m_lightingTintButton = new ColorPickerButton(Qt::white);
    m_lightingTintButton->setToolTip(tr("Lighting color tint"));
    tintLayout->addWidget(m_lightingTintButton);
    tintLayout->addStretch();

    m_lightingSection->addWidget(tintRow);

    // DM-only section label
    QLabel* dmOnlyLabel = new QLabel(tr("DM View Only:"));
    dmOnlyLabel->setStyleSheet("color: #808080; font-size: 10px; margin-top: 8px;");
    m_lightingSection->addWidget(dmOnlyLabel);

    // Brightness slider (DM only)
    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setRange(-100, 100);  // -1.0 to 1.0
    m_brightnessSlider->setValue(0);
    m_brightnessSlider->setToolTip(tr("Brightness adjustment (DM view only)"));
    m_brightnessValue = new QLabel("0%");
    m_lightingSection->addWidget(createSliderRow(tr("Brightness"), m_brightnessSlider, m_brightnessValue));

    // Contrast slider (DM only)
    m_contrastSlider = new QSlider(Qt::Horizontal);
    m_contrastSlider->setRange(-100, 100);  // -1.0 to 1.0
    m_contrastSlider->setValue(0);
    m_contrastSlider->setToolTip(tr("Contrast adjustment (DM view only)"));
    m_contrastValue = new QLabel("0%");
    m_lightingSection->addWidget(createSliderRow(tr("Contrast"), m_contrastSlider, m_contrastValue));

    m_mainLayout->addWidget(m_lightingSection);
}

void AtmosphereToolboxWidget::createWeatherSection()
{
    m_weatherSection = new ToolSection(tr("Weather"), m_contentWidget);
    m_weatherSection->setCollapsible(true);
    m_weatherSection->setExpanded(false);

    // Weather type combo
    QWidget* typeRow = new QWidget();
    QHBoxLayout* typeLayout = new QHBoxLayout(typeRow);
    typeLayout->setContentsMargins(0, 0, 0, 0);
    typeLayout->setSpacing(8);

    QLabel* typeLabel = new QLabel(tr("Type"));
    typeLabel->setFixedWidth(70);
    typeLabel->setStyleSheet("color: #A0A0A0; font-size: 11px;");
    typeLayout->addWidget(typeLabel);

    m_weatherTypeCombo = new QComboBox();
    m_weatherTypeCombo->addItem(tr("None"), 0);
    m_weatherTypeCombo->addItem(tr("Rain"), 1);
    m_weatherTypeCombo->addItem(tr("Snow"), 2);
    m_weatherTypeCombo->addItem(tr("Storm"), 3);
    m_weatherTypeCombo->setToolTip(tr("Weather effect type"));
    typeLayout->addWidget(m_weatherTypeCombo, 1);

    m_weatherSection->addWidget(typeRow);

    // Intensity slider
    m_weatherIntensitySlider = new QSlider(Qt::Horizontal);
    m_weatherIntensitySlider->setRange(0, 100);
    m_weatherIntensitySlider->setValue(50);
    m_weatherIntensitySlider->setToolTip(tr("Precipitation density"));
    m_weatherIntensityValue = new QLabel("50%");
    m_weatherSection->addWidget(createSliderRow(tr("Intensity"), m_weatherIntensitySlider, m_weatherIntensityValue));

    // Wind strength slider
    m_windStrengthSlider = new QSlider(Qt::Horizontal);
    m_windStrengthSlider->setRange(0, 100);
    m_windStrengthSlider->setValue(30);
    m_windStrengthSlider->setToolTip(tr("Wind strength affects particle direction"));
    m_windStrengthValue = new QLabel("30%");
    m_weatherSection->addWidget(createSliderRow(tr("Wind"), m_windStrengthSlider, m_windStrengthValue));

    m_mainLayout->addWidget(m_weatherSection);
}

void AtmosphereToolboxWidget::createFogSection()
{
    m_fogSection = new ToolSection(tr("Fog / Mist"), m_contentWidget);
    m_fogSection->setCollapsible(true);
    m_fogSection->setExpanded(false);

    // Enable checkbox
    m_fogEnabledCheckbox = new QCheckBox(tr("Enable Fog"));
    m_fogEnabledCheckbox->setToolTip(tr("Toggle atmospheric fog/mist effect"));
    m_fogSection->addWidget(m_fogEnabledCheckbox);

    // Density slider
    m_fogDensitySlider = new QSlider(Qt::Horizontal);
    m_fogDensitySlider->setRange(0, 100);
    m_fogDensitySlider->setValue(30);
    m_fogDensitySlider->setToolTip(tr("Fog thickness/opacity"));
    m_fogDensityValue = new QLabel("30%");
    m_fogSection->addWidget(createSliderRow(tr("Density"), m_fogDensitySlider, m_fogDensityValue));

    // Height slider
    m_fogHeightSlider = new QSlider(Qt::Horizontal);
    m_fogHeightSlider->setRange(0, 100);
    m_fogHeightSlider->setValue(30);
    m_fogHeightSlider->setToolTip(tr("How much of the scene fog covers from bottom"));
    m_fogHeightValue = new QLabel("30%");
    m_fogSection->addWidget(createSliderRow(tr("Height"), m_fogHeightSlider, m_fogHeightValue));

    // Color picker
    QWidget* colorRow = new QWidget();
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(8);

    QLabel* colorLabel = new QLabel(tr("Color"));
    colorLabel->setFixedWidth(70);
    colorLabel->setStyleSheet("color: #A0A0A0; font-size: 11px;");
    colorLayout->addWidget(colorLabel);

    m_fogColorButton = new ColorPickerButton(QColor(180, 180, 200, 128));
    m_fogColorButton->setAlphaEnabled(true);
    m_fogColorButton->setToolTip(tr("Fog color and opacity"));
    colorLayout->addWidget(m_fogColorButton);
    colorLayout->addStretch();

    m_fogSection->addWidget(colorRow);

    // Animation speed slider
    m_fogAnimationSlider = new QSlider(Qt::Horizontal);
    m_fogAnimationSlider->setRange(0, 100);
    m_fogAnimationSlider->setValue(50);
    m_fogAnimationSlider->setToolTip(tr("Fog movement speed"));
    m_fogAnimationValue = new QLabel("50%");
    m_fogSection->addWidget(createSliderRow(tr("Animation"), m_fogAnimationSlider, m_fogAnimationValue));

    // Twist amount slider
    m_fogTwistSlider = new QSlider(Qt::Horizontal);
    m_fogTwistSlider->setRange(0, 100);
    m_fogTwistSlider->setValue(30);
    m_fogTwistSlider->setToolTip(tr("Amount of swirling/rotation in fog"));
    m_fogTwistValue = new QLabel("30%");
    m_fogSection->addWidget(createSliderRow(tr("Twist"), m_fogTwistSlider, m_fogTwistValue));

    m_mainLayout->addWidget(m_fogSection);
}

void AtmosphereToolboxWidget::createLightningSection()
{
    m_lightningSection = new ToolSection(tr("Lightning"), m_contentWidget);
    m_lightningSection->setCollapsible(true);
    m_lightningSection->setExpanded(false);

    // Enable checkbox
    m_lightningEnabledCheckbox = new QCheckBox(tr("Enable Lightning"));
    m_lightningEnabledCheckbox->setToolTip(tr("Toggle lightning flash effects"));
    m_lightningSection->addWidget(m_lightningEnabledCheckbox);

    // Intensity slider
    m_lightningIntensitySlider = new QSlider(Qt::Horizontal);
    m_lightningIntensitySlider->setRange(0, 100);
    m_lightningIntensitySlider->setValue(80);
    m_lightningIntensitySlider->setToolTip(tr("Maximum flash brightness"));
    m_lightningIntensityValue = new QLabel("80%");
    m_lightningSection->addWidget(createSliderRow(tr("Intensity"), m_lightningIntensitySlider, m_lightningIntensityValue));

    // Frequency slider
    m_lightningFrequencySlider = new QSlider(Qt::Horizontal);
    m_lightningFrequencySlider->setRange(0, 100);
    m_lightningFrequencySlider->setValue(50);
    m_lightningFrequencySlider->setToolTip(tr("How often lightning strikes occur"));
    m_lightningFrequencyValue = new QLabel("50%");
    m_lightningSection->addWidget(createSliderRow(tr("Frequency"), m_lightningFrequencySlider, m_lightningFrequencyValue));

    m_mainLayout->addWidget(m_lightningSection);
}

void AtmosphereToolboxWidget::createAudioSection()
{
    m_audioSection = new ToolSection(tr("Audio"), m_contentWidget);
    m_audioSection->setCollapsible(true);
    m_audioSection->setExpanded(true);

    // --- Ambient Sound subsection ---
    QLabel* ambientHeader = new QLabel(tr("Ambient Sound"));
    ambientHeader->setStyleSheet("font-weight: bold; color: #E0E0E0;");
    m_audioSection->addWidget(ambientHeader);

    m_ambientTrackLabel = new QLabel(tr("No track loaded"));
    m_ambientTrackLabel->setStyleSheet("color: #808080; font-style: italic;");
    m_ambientTrackLabel->setWordWrap(true);
    m_audioSection->addWidget(m_ambientTrackLabel);

    // Ambient volume
    m_ambientVolumeSlider = new QSlider(Qt::Horizontal);
    m_ambientVolumeSlider->setRange(0, 100);
    m_ambientVolumeSlider->setValue(70);
    m_ambientVolumeValue = new QLabel("70%");
    m_audioSection->addWidget(createSliderRow(tr("Volume"), m_ambientVolumeSlider, m_ambientVolumeValue));

    // Browse / Stop buttons
    QWidget* ambientButtonRow = new QWidget();
    QHBoxLayout* ambientButtons = new QHBoxLayout(ambientButtonRow);
    ambientButtons->setContentsMargins(0, 0, 0, 0);
    ambientButtons->setSpacing(4);
    m_ambientBrowseButton = new QPushButton(tr("Browse..."));
    m_ambientStopButton = new QPushButton(tr("Stop"));
    m_ambientStopButton->setEnabled(false);
    ambientButtons->addWidget(m_ambientBrowseButton);
    ambientButtons->addWidget(m_ambientStopButton);
    m_audioSection->addWidget(ambientButtonRow);

    // --- Separator ---
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color: #3A3A3A;");
    m_audioSection->addWidget(separator);

    // --- Music Remote subsection ---
    QLabel* musicHeader = new QLabel(tr("Music Remote"));
    musicHeader->setStyleSheet("font-weight: bold; color: #E0E0E0;");
    m_audioSection->addWidget(musicHeader);

    m_nowPlayingLabel = new QLabel(tr("No music playing"));
    m_nowPlayingLabel->setStyleSheet("color: #808080;");
    m_nowPlayingLabel->setWordWrap(true);
    m_audioSection->addWidget(m_nowPlayingLabel);

    m_nowPlayingArtist = new QLabel("");
    m_nowPlayingArtist->setStyleSheet("color: #808080;");
    m_audioSection->addWidget(m_nowPlayingArtist);

    // Transport controls
    QWidget* transportRow = new QWidget();
    QHBoxLayout* transportLayout = new QHBoxLayout(transportRow);
    transportLayout->setContentsMargins(0, 0, 0, 0);
    transportLayout->setSpacing(4);
    m_musicPrevButton = new QPushButton(QString::fromUtf8("\xe2\x8f\xae"));
    m_musicPlayPauseButton = new QPushButton(QString::fromUtf8("\xe2\x8f\xaf"));
    m_musicNextButton = new QPushButton(QString::fromUtf8("\xe2\x8f\xad"));
    for (auto* btn : {m_musicPrevButton, m_musicPlayPauseButton, m_musicNextButton}) {
        btn->setFixedSize(36, 36);
        btn->setStyleSheet("QPushButton { font-size: 16px; }");
    }
    transportLayout->addStretch();
    transportLayout->addWidget(m_musicPrevButton);
    transportLayout->addWidget(m_musicPlayPauseButton);
    transportLayout->addWidget(m_musicNextButton);
    transportLayout->addStretch();
    m_audioSection->addWidget(transportRow);

    // Music volume
    m_musicVolumeSlider = new QSlider(Qt::Horizontal);
    m_musicVolumeSlider->setRange(0, 100);
    m_musicVolumeSlider->setValue(60);
    m_musicVolumeValue = new QLabel("60%");
    m_audioSection->addWidget(createSliderRow(tr("Volume"), m_musicVolumeSlider, m_musicVolumeValue));

    m_mainLayout->addWidget(m_audioSection);
}

QWidget* AtmosphereToolboxWidget::createSliderRow(const QString& labelText, QSlider* slider, QLabel* valueLabel)
{
    QWidget* row = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(8);

    QLabel* label = new QLabel(labelText);
    label->setFixedWidth(70);
    label->setStyleSheet("color: #A0A0A0; font-size: 11px;");
    layout->addWidget(label);

    layout->addWidget(slider, 1);

    valueLabel->setFixedWidth(40);
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    valueLabel->setStyleSheet("color: #E0E0E0; font-size: 11px;");
    layout->addWidget(valueLabel);

    return row;
}

void AtmosphereToolboxWidget::applyDarkTheme()
{
    setStyleSheet(QString(R"(
        QDockWidget {
            background-color: %1;
            color: %2;
            font-size: 12px;
        }
        QDockWidget::title {
            background-color: %3;
            padding: 6px;
            font-weight: bold;
        }
        QScrollArea {
            background-color: transparent;
            border: none;
        }
        QComboBox {
            background-color: %4;
            border: 1px solid %5;
            border-radius: 4px;
            padding: 4px 8px;
            color: %2;
            min-height: 24px;
        }
        QComboBox:hover {
            border-color: %6;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: %3;
            border: 1px solid %5;
            selection-background-color: %6;
        }
        QPushButton {
            background-color: %4;
            border: 1px solid %5;
            border-radius: 4px;
            padding: 4px 12px;
            color: %2;
            min-height: 24px;
        }
        QPushButton:hover {
            background-color: %7;
            border-color: %6;
        }
        QPushButton:pressed {
            background-color: %6;
        }
        QPushButton:checked {
            background-color: %6;
            border-color: %6;
        }
        QPushButton:disabled {
            background-color: %4;
            color: %8;
        }
        QSlider::groove:horizontal {
            background: %4;
            height: 6px;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: %6;
            width: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background: #5BA3F5;
        }
        QSlider::sub-page:horizontal {
            background: %6;
            border-radius: 3px;
        }
        QCheckBox {
            color: %2;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %5;
            border-radius: 3px;
            background: %4;
        }
        QCheckBox::indicator:checked {
            background: %6;
            border-color: %6;
        }
        QCheckBox::indicator:hover {
            border-color: %6;
        }
    )")
    .arg(DarkTheme::BgPrimary.name())      // %1
    .arg(DarkTheme::TextPrimary.name())    // %2
    .arg(DarkTheme::BgSecondary.name())    // %3
    .arg(DarkTheme::BgTertiary.name())     // %4
    .arg(DarkTheme::BorderColor.name())    // %5
    .arg(DarkTheme::AccentPrimary.name())  // %6
    .arg("#3A3A3A")                         // %7 hover
    .arg(DarkTheme::TextDisabled.name())); // %8
}

void AtmosphereToolboxWidget::populatePresetCombo()
{
    m_presetCombo->clear();

    // Add built-in presets
    auto presets = AtmospherePreset::getBuiltInPresets();
    for (const auto& preset : presets) {
        m_presetCombo->addItem(preset.name, preset.name);
    }

    // TODO: Add custom presets from CustomPresetManager
}

void AtmosphereToolboxWidget::connectSignals()
{
    // Preset controls
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AtmosphereToolboxWidget::onPresetComboChanged);
    connect(m_timeOfDayGroup, &QButtonGroup::idClicked,
            this, &AtmosphereToolboxWidget::onTimeOfDayClicked);
    connect(m_revertButton, &QPushButton::clicked,
            this, &AtmosphereToolboxWidget::onRevertClicked);
    connect(m_savePresetButton, &QPushButton::clicked,
            this, &AtmosphereToolboxWidget::savePresetRequested);

    // Lighting controls
    connect(m_lightingIntensitySlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onLightingIntensityChanged);
    connect(m_ambientSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onAmbientLevelChanged);
    connect(m_exposureSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onExposureChanged);
    connect(m_lightingTintButton, &ColorPickerButton::colorChanged,
            this, &AtmosphereToolboxWidget::onLightingTintChanged);

    // DM-only brightness/contrast controls
    connect(m_brightnessSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onBrightnessChanged);
    connect(m_contrastSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onContrastChanged);

    // Weather controls
    connect(m_weatherTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AtmosphereToolboxWidget::onWeatherTypeChanged);
    connect(m_weatherIntensitySlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onWeatherIntensityChanged);
    connect(m_windStrengthSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onWindStrengthChanged);

    // Fog controls
    connect(m_fogEnabledCheckbox, &QCheckBox::toggled,
            this, &AtmosphereToolboxWidget::onFogEnabledChanged);
    connect(m_fogDensitySlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onFogDensityChanged);
    connect(m_fogHeightSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onFogHeightChanged);
    connect(m_fogColorButton, &ColorPickerButton::colorChanged,
            this, &AtmosphereToolboxWidget::onFogColorChanged);
    connect(m_fogAnimationSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onFogAnimationChanged);
    connect(m_fogTwistSlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onFogTwistChanged);

    // Lightning controls
    connect(m_lightningEnabledCheckbox, &QCheckBox::toggled,
            this, &AtmosphereToolboxWidget::onLightningEnabledChanged);
    connect(m_lightningIntensitySlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onLightningIntensityChanged);
    connect(m_lightningFrequencySlider, &QSlider::valueChanged,
            this, &AtmosphereToolboxWidget::onLightningFrequencyChanged);

    // Audio connections — only wired when createAudioSection() built the
    // widgets. Audio is currently out of scope; the section is not created
    // and these pointers stay nullptr.
    if (m_ambientVolumeSlider) {
        connect(m_ambientVolumeSlider, &QSlider::valueChanged,
                this, &AtmosphereToolboxWidget::onAmbientVolumeChanged);
        connect(m_ambientBrowseButton, &QPushButton::clicked,
                this, &AtmosphereToolboxWidget::onAmbientBrowseClicked);
        connect(m_ambientStopButton, &QPushButton::clicked,
                this, &AtmosphereToolboxWidget::onAmbientStopClicked);
        connect(m_musicPrevButton, &QPushButton::clicked,
                this, &AtmosphereToolboxWidget::onMusicPrevClicked);
        connect(m_musicPlayPauseButton, &QPushButton::clicked,
                this, &AtmosphereToolboxWidget::onMusicPlayPauseClicked);
        connect(m_musicNextButton, &QPushButton::clicked,
                this, &AtmosphereToolboxWidget::onMusicNextClicked);
        connect(m_musicVolumeSlider, &QSlider::valueChanged,
                this, &AtmosphereToolboxWidget::onMusicVolumeChanged);
    }
}

void AtmosphereToolboxWidget::updateFromState(const AtmosphereState& state)
{
    // Skip if we're already updating or if we're the source of this change
    if (m_updatingFromState || m_applyingState) return;
    m_updatingFromState = true;

    // Update time of day buttons
    QAbstractButton* btn = m_timeOfDayGroup->button(state.timeOfDay);
    if (btn) btn->setChecked(true);

    // Lighting
    m_lightingIntensitySlider->setValue(qRound(state.lightingIntensity * 100));
    m_ambientSlider->setValue(qRound(state.ambientLevel * 100));
    m_exposureSlider->setValue(qRound(state.exposure * 100));
    m_lightingTintButton->setColor(state.lightingTint);

    // Weather
    m_weatherTypeCombo->setCurrentIndex(state.weatherType);
    m_weatherIntensitySlider->setValue(qRound(state.weatherIntensity * 100));
    m_windStrengthSlider->setValue(qRound(state.windStrength * 100));

    // Fog
    m_fogEnabledCheckbox->setChecked(state.fogEnabled);
    m_fogDensitySlider->setValue(qRound(state.fogDensity * 100));
    m_fogHeightSlider->setValue(qRound(state.fogHeight * 100));
    m_fogColorButton->setColor(state.fogColor);

    // Lightning
    m_lightningEnabledCheckbox->setChecked(state.lightningEnabled);
    m_lightningIntensitySlider->setValue(qRound(state.lightningIntensity * 100));
    m_lightningFrequencySlider->setValue(qRound(state.lightningFrequency * 100));

    m_updatingFromState = false;
}

void AtmosphereToolboxWidget::onPresetChanged(const QString& presetName)
{
    m_currentPresetName = presetName;

    // Update combo box selection
    int index = m_presetCombo->findData(presetName);
    if (index >= 0) {
        m_presetCombo->blockSignals(true);
        m_presetCombo->setCurrentIndex(index);
        m_presetCombo->blockSignals(false);
    }

    // Store base state for revert
    m_presetBaseState = AtmospherePreset::getPresetByName(presetName).state;
    m_isModified = false;
    updateModifiedState();
}

void AtmosphereToolboxWidget::onPresetComboChanged(int index)
{
    if (m_updatingFromState || index < 0) return;

    QString presetName = m_presetCombo->itemData(index).toString();
    if (!presetName.isEmpty() && m_atmosphereManager) {
        m_atmosphereManager->applyPreset(presetName);
    }
}

void AtmosphereToolboxWidget::onTimeOfDayClicked(int id)
{
    if (m_updatingFromState || !m_atmosphereManager) return;

    m_atmosphereManager->setTimeOfDay(id);
    updateModifiedState();
}

void AtmosphereToolboxWidget::onRevertClicked()
{
    if (m_atmosphereManager && !m_currentPresetName.isEmpty()) {
        m_atmosphereManager->applyPreset(m_currentPresetName);
    }
}

void AtmosphereToolboxWidget::onLightingIntensityChanged(int value)
{
    m_lightingIntensityValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onAmbientLevelChanged(int value)
{
    m_ambientValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onExposureChanged(int value)
{
    m_exposureValue->setText(QString::number(value / 100.0, 'f', 1));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onLightingTintChanged(const QColor& /*color*/)
{
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onBrightnessChanged(int value)
{
    m_brightnessValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState && m_atmosphereManager) {
        // Apply brightness directly to LightingOverlay (DM-only)
        MapDisplay* display = m_atmosphereManager->getMapDisplay();
        if (display && display->getLightingOverlay()) {
            display->getLightingOverlay()->setBrightness(value / 100.0);
        }
    }
}

void AtmosphereToolboxWidget::onContrastChanged(int value)
{
    m_contrastValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState && m_atmosphereManager) {
        // Apply contrast directly to LightingOverlay (DM-only)
        MapDisplay* display = m_atmosphereManager->getMapDisplay();
        if (display && display->getLightingOverlay()) {
            display->getLightingOverlay()->setContrast(value / 100.0);
        }
    }
}

void AtmosphereToolboxWidget::onWeatherTypeChanged(int /*index*/)
{
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onWeatherIntensityChanged(int value)
{
    m_weatherIntensityValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onWindStrengthChanged(int value)
{
    m_windStrengthValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onFogEnabledChanged(bool /*enabled*/)
{
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onFogDensityChanged(int value)
{
    m_fogDensityValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onFogHeightChanged(int value)
{
    m_fogHeightValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onFogColorChanged(const QColor& /*color*/)
{
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onFogAnimationChanged(int value)
{
    m_fogAnimationValue->setText(QString("%1%").arg(value));
    m_fogAnimationSpeed = value / 100.0;

    // Debounce the fog effect update
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onFogTwistChanged(int value)
{
    m_fogTwistValue->setText(QString("%1%").arg(value));
    m_fogTwistAmount = value / 100.0;

    // Debounce the fog effect update
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onLightningEnabledChanged(bool /*enabled*/)
{
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onLightningIntensityChanged(int value)
{
    m_lightningIntensityValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

void AtmosphereToolboxWidget::onLightningFrequencyChanged(int value)
{
    m_lightningFrequencyValue->setText(QString("%1%").arg(value));
    if (!m_updatingFromState) {
        scheduleStateUpdate();
    }
}

AtmosphereState AtmosphereToolboxWidget::buildStateFromControls() const
{
    AtmosphereState state;

    // Time of day
    int tod = m_timeOfDayGroup->checkedId();
    state.timeOfDay = (tod >= 0) ? tod : 1;

    // Lighting
    state.lightingIntensity = m_lightingIntensitySlider->value() / 100.0;
    state.ambientLevel = m_ambientSlider->value() / 100.0;
    state.exposure = m_exposureSlider->value() / 100.0;
    state.lightingTint = m_lightingTintButton->color();

    // Weather
    state.weatherType = m_weatherTypeCombo->currentIndex();
    state.weatherIntensity = m_weatherIntensitySlider->value() / 100.0;
    state.windStrength = m_windStrengthSlider->value() / 100.0;

    // Fog
    state.fogEnabled = m_fogEnabledCheckbox->isChecked();
    state.fogDensity = m_fogDensitySlider->value() / 100.0;
    state.fogHeight = m_fogHeightSlider->value() / 100.0;
    state.fogColor = m_fogColorButton->color();

    // Lightning
    state.lightningEnabled = m_lightningEnabledCheckbox->isChecked();
    state.lightningIntensity = m_lightningIntensitySlider->value() / 100.0;
    state.lightningFrequency = m_lightningFrequencySlider->value() / 100.0;

    return state;
}

void AtmosphereToolboxWidget::applyCurrentState()
{
    if (!m_atmosphereManager) return;

    // Set flag to prevent feedback loop from stateChanged signal
    m_applyingState = true;

    AtmosphereState state = buildStateFromControls();
    m_atmosphereManager->setStateImmediate(state);

    // Apply fog animation/twist directly (not in AtmosphereState)
    if (m_atmosphereManager->getMapDisplay()) {
        if (auto* fog = m_atmosphereManager->getMapDisplay()->getFogMistEffect()) {
            fog->setAnimationSpeed(m_fogAnimationSpeed);
            fog->setTextureTwist(m_fogTwistAmount);
        }
    }

    m_applyingState = false;

    updateModifiedState();
}

void AtmosphereToolboxWidget::scheduleStateUpdate()
{
    // Restart the debounce timer - actual update happens after DEBOUNCE_MS idle time
    // This prevents expensive operations during rapid slider dragging
    if (m_debounceTimer) {
        m_debounceTimer->start();
    }
    // Note: updateModifiedState() is called in applyCurrentState() after debounce fires
    // Removing it here improves performance during slider dragging
}

void AtmosphereToolboxWidget::updateModifiedState()
{
    AtmosphereState current = buildStateFromControls();
    m_isModified = (current != m_presetBaseState);

    if (m_isModified) {
        m_modifiedLabel->setText(tr("(Modified)"));
        m_modifiedLabel->show();
        m_revertButton->setEnabled(true);
    } else {
        m_modifiedLabel->hide();
        m_revertButton->setEnabled(false);
    }
}

void AtmosphereToolboxWidget::setAudioSystems(AmbientPlayer* ambient, MusicRemote* remote)
{
    m_ambientPlayer = ambient;
    m_musicRemote = remote;

    // The audio panel is currently not built (createAudioSection() is not
    // called). Guard so connecting real audio systems without the UI in place
    // can never crash on a nullptr widget deref inside the lambdas below.
    if (!m_audioSection) return;

    if (m_ambientPlayer) {
        connect(m_ambientPlayer, &AmbientPlayer::trackChanged, this, [this](const QString& track) {
            if (track.isEmpty()) {
                m_ambientTrackLabel->setText(tr("No track loaded"));
                m_ambientTrackLabel->setStyleSheet("color: #808080; font-style: italic;");
                m_ambientStopButton->setEnabled(false);
            } else {
                m_ambientTrackLabel->setText(QFileInfo(track).fileName());
                m_ambientTrackLabel->setStyleSheet("color: #4A90E2;");
                m_ambientStopButton->setEnabled(true);
            }
        });
        connect(m_ambientPlayer, &AmbientPlayer::volumeChanged, this, [this](qreal vol) {
            m_ambientVolumeSlider->blockSignals(true);
            m_ambientVolumeSlider->setValue(static_cast<int>(vol * 100));
            m_ambientVolumeSlider->blockSignals(false);
            m_ambientVolumeValue->setText(QString("%1%").arg(static_cast<int>(vol * 100)));
        });
    }

    if (m_musicRemote) {
        connect(m_musicRemote, &MusicRemote::nowPlayingChanged, this, [this](const MusicRemote::NowPlaying&) {
            onNowPlayingChanged();
        });
        m_musicRemote->startPolling(2000);
    }
}

void AtmosphereToolboxWidget::onAmbientVolumeChanged(int value)
{
    if (m_ambientPlayer) {
        m_ambientPlayer->setVolume(value / 100.0);
    }
    m_ambientVolumeValue->setText(QString("%1%").arg(value));
}

void AtmosphereToolboxWidget::onAmbientBrowseClicked()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString file = QFileDialog::getOpenFileName(this, tr("Select Ambient Sound"),
        dir, tr("Audio Files (*.mp3 *.ogg *.wav *.flac *.m4a);;All Files (*)"));
    if (!file.isEmpty() && m_ambientPlayer) {
        m_ambientPlayer->play(file);
    }
}

void AtmosphereToolboxWidget::onAmbientStopClicked()
{
    if (m_ambientPlayer) {
        m_ambientPlayer->stop();
    }
}

void AtmosphereToolboxWidget::onMusicPlayPauseClicked()
{
    if (m_musicRemote) m_musicRemote->playPause();
}

void AtmosphereToolboxWidget::onMusicNextClicked()
{
    if (m_musicRemote) m_musicRemote->nextTrack();
}

void AtmosphereToolboxWidget::onMusicPrevClicked()
{
    if (m_musicRemote) m_musicRemote->previousTrack();
}

void AtmosphereToolboxWidget::onMusicVolumeChanged(int value)
{
    if (m_musicRemote) m_musicRemote->setVolume(value / 100.0);
    m_musicVolumeValue->setText(QString("%1%").arg(value));
}

void AtmosphereToolboxWidget::onNowPlayingChanged()
{
    if (!m_musicRemote) return;
    auto np = m_musicRemote->getNowPlaying();
    if (np.title.isEmpty()) {
        m_nowPlayingLabel->setText(tr("No music playing"));
        m_nowPlayingLabel->setStyleSheet("color: #808080;");
        m_nowPlayingArtist->setText("");
    } else {
        m_nowPlayingLabel->setText(np.title);
        m_nowPlayingLabel->setStyleSheet("color: #E0E0E0;");
        m_nowPlayingArtist->setText(np.artist);
    }
}
