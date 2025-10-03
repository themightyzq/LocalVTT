#include "ui/dialogs/SettingsDialog.h"
#include "utils/SettingsManager.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QButtonGroup>
#include <QRadioButton>
#include <QApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QFormLayout>

// Define static default colors
const QColor SettingsDialog::DEFAULT_GM_BEACON_COLOR = QColor(74, 158, 255); // Accent blue
const QColor SettingsDialog::DEFAULT_GRID_COLOR = QColor(255, 255, 255, 128); // Semi-transparent white

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_gmBeaconTab(nullptr)
    , m_gmBeaconSizeSlider(nullptr)
    , m_gmBeaconSizeLabel(nullptr)
    , m_gmBeaconColorButton(nullptr)
    , m_gmBeaconColor(DEFAULT_GM_BEACON_COLOR)
    , m_gmBeaconShapeGroup(nullptr)
    , m_circleShapeRadio(nullptr)
    , m_starShapeRadio(nullptr)
    , m_crosshairShapeRadio(nullptr)
    , m_gmBeaconOpacitySlider(nullptr)
    , m_gmBeaconOpacityLabel(nullptr)
    , m_fogMistTab(nullptr)
    , m_fogAnimationSpeedSlider(nullptr)
    , m_fogAnimationSpeedLabel(nullptr)
    , m_fogOpacitySlider(nullptr)
    , m_fogOpacityLabel(nullptr)
    , m_fogTextureCombo(nullptr)
    , m_performanceTab(nullptr)
    , m_animationQualityCombo(nullptr)
    , m_smoothAnimationsCheck(nullptr)
    , m_updateFrequencySlider(nullptr)
    , m_updateFrequencyLabel(nullptr)
    , m_displayTab(nullptr)
    , m_gridOpacitySlider(nullptr)
    , m_gridOpacityLabel(nullptr)
    , m_gridColorButton(nullptr)
    , m_gridColor(DEFAULT_GRID_COLOR)
    , m_defaultFogBrushSlider(nullptr)
    , m_defaultFogBrushLabel(nullptr)
    , m_buttonBox(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_applyButton(nullptr)
    , m_resetButton(nullptr)
    , m_colorDialog(nullptr)
{
    setWindowTitle("LocalVTT Preferences");
    setModal(true);
    setMinimumSize(500, 400);
    resize(600, 500);

    setupUI();
    loadSettings();
    connectSignals();

    // Apply theme

    // Apply custom dialog styling
}

SettingsDialog::~SettingsDialog()
{
    // Cleanup handled by Qt parent-child hierarchy
}

void SettingsDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);

    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);

    // Create tabs
    createGMBeaconTab();
    createFogMistTab();
    createPerformanceTab();
    createDisplayTab();

    m_mainLayout->addWidget(m_tabWidget);

    // Create button box
    createButtonBox();
    m_mainLayout->addWidget(m_buttonBox);

    // Apply tab widget styling
}

void SettingsDialog::createGMBeaconTab()
{
    m_gmBeaconTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_gmBeaconTab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    // Size settings
    QGroupBox* sizeGroup = new QGroupBox("Beacon Size");
    QFormLayout* sizeLayout = new QFormLayout(sizeGroup);

    m_gmBeaconSizeSlider = new QSlider(Qt::Horizontal);
    m_gmBeaconSizeSlider->setRange(10, 50);
    m_gmBeaconSizeSlider->setValue(DEFAULT_GM_BEACON_SIZE);
    m_gmBeaconSizeLabel = new QLabel(QString("%1% of viewport").arg(DEFAULT_GM_BEACON_SIZE));

    QHBoxLayout* sizeSliderLayout = new QHBoxLayout();
    sizeSliderLayout->addWidget(m_gmBeaconSizeSlider);
    sizeSliderLayout->addWidget(m_gmBeaconSizeLabel);
    sizeLayout->addRow("Size:", sizeSliderLayout);

    // Color settings
    QGroupBox* colorGroup = new QGroupBox("Beacon Color");
    QFormLayout* colorLayout = new QFormLayout(colorGroup);

    m_gmBeaconColorButton = new QPushButton();
    m_gmBeaconColorButton->setFixedSize(50, 30);
    updateColorButton(m_gmBeaconColorButton, m_gmBeaconColor);
    colorLayout->addRow("Color:", m_gmBeaconColorButton);

    // Shape settings
    QGroupBox* shapeGroup = new QGroupBox("Beacon Shape");
    QVBoxLayout* shapeLayout = new QVBoxLayout(shapeGroup);

    m_gmBeaconShapeGroup = new QButtonGroup(this);
    m_circleShapeRadio = new QRadioButton("Circle");
    m_starShapeRadio = new QRadioButton("Star");
    m_crosshairShapeRadio = new QRadioButton("Crosshair");

    m_gmBeaconShapeGroup->addButton(m_circleShapeRadio, 0);
    m_gmBeaconShapeGroup->addButton(m_starShapeRadio, 1);
    m_gmBeaconShapeGroup->addButton(m_crosshairShapeRadio, 2);

    m_circleShapeRadio->setChecked(true); // Default

    shapeLayout->addWidget(m_circleShapeRadio);
    shapeLayout->addWidget(m_starShapeRadio);
    shapeLayout->addWidget(m_crosshairShapeRadio);

    // Opacity settings
    QGroupBox* opacityGroup = new QGroupBox("Beacon Opacity");
    QFormLayout* opacityLayout = new QFormLayout(opacityGroup);

    m_gmBeaconOpacitySlider = new QSlider(Qt::Horizontal);
    m_gmBeaconOpacitySlider->setRange(10, 100);
    m_gmBeaconOpacitySlider->setValue(DEFAULT_GM_BEACON_OPACITY);
    m_gmBeaconOpacityLabel = new QLabel(QString("%1%").arg(DEFAULT_GM_BEACON_OPACITY));

    QHBoxLayout* opacitySliderLayout = new QHBoxLayout();
    opacitySliderLayout->addWidget(m_gmBeaconOpacitySlider);
    opacitySliderLayout->addWidget(m_gmBeaconOpacityLabel);
    opacityLayout->addRow("Opacity:", opacitySliderLayout);

    layout->addWidget(sizeGroup);
    layout->addWidget(colorGroup);
    layout->addWidget(shapeGroup);
    layout->addWidget(opacityGroup);
    layout->addStretch();

    m_tabWidget->addTab(m_gmBeaconTab, "GM Beacon");
}

void SettingsDialog::createFogMistTab()
{
    m_fogMistTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_fogMistTab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    // Animation speed settings
    QGroupBox* animationGroup = new QGroupBox("Animation Settings");
    QFormLayout* animationLayout = new QFormLayout(animationGroup);

    m_fogAnimationSpeedSlider = new QSlider(Qt::Horizontal);
    m_fogAnimationSpeedSlider->setRange(10, 100);
    m_fogAnimationSpeedSlider->setValue(DEFAULT_FOG_ANIMATION_SPEED);
    m_fogAnimationSpeedLabel = new QLabel(QString("%1%").arg(DEFAULT_FOG_ANIMATION_SPEED));

    QHBoxLayout* animationSliderLayout = new QHBoxLayout();
    animationSliderLayout->addWidget(m_fogAnimationSpeedSlider);
    animationSliderLayout->addWidget(m_fogAnimationSpeedLabel);
    animationLayout->addRow("Animation Speed:", animationSliderLayout);

    // Opacity settings
    QGroupBox* fogOpacityGroup = new QGroupBox("Fog Opacity");
    QFormLayout* fogOpacityLayout = new QFormLayout(fogOpacityGroup);

    m_fogOpacitySlider = new QSlider(Qt::Horizontal);
    m_fogOpacitySlider->setRange(10, 100);
    m_fogOpacitySlider->setValue(DEFAULT_FOG_OPACITY);
    m_fogOpacityLabel = new QLabel(QString("%1%").arg(DEFAULT_FOG_OPACITY));

    QHBoxLayout* fogOpacitySliderLayout = new QHBoxLayout();
    fogOpacitySliderLayout->addWidget(m_fogOpacitySlider);
    fogOpacitySliderLayout->addWidget(m_fogOpacityLabel);
    fogOpacityLayout->addRow("Fog Opacity:", fogOpacitySliderLayout);

    // Texture settings
    QGroupBox* textureGroup = new QGroupBox("Fog Texture");
    QFormLayout* textureLayout = new QFormLayout(textureGroup);

    m_fogTextureCombo = new QComboBox();
    m_fogTextureCombo->addItems({"Solid", "Wispy", "Dense", "Animated"});
    m_fogTextureCombo->setCurrentIndex(DEFAULT_FOG_TEXTURE);
    textureLayout->addRow("Texture Type:", m_fogTextureCombo);

    layout->addWidget(animationGroup);
    layout->addWidget(fogOpacityGroup);
    layout->addWidget(textureGroup);
    layout->addStretch();

    m_tabWidget->addTab(m_fogMistTab, "Fog/Mist");
}

void SettingsDialog::createPerformanceTab()
{
    m_performanceTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_performanceTab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    // Animation quality settings
    QGroupBox* qualityGroup = new QGroupBox("Animation Quality");
    QFormLayout* qualityLayout = new QFormLayout(qualityGroup);

    m_animationQualityCombo = new QComboBox();
    m_animationQualityCombo->addItems({"Low", "Medium", "High"});
    m_animationQualityCombo->setCurrentIndex(DEFAULT_ANIMATION_QUALITY);
    qualityLayout->addRow("Quality Level:", m_animationQualityCombo);

    m_smoothAnimationsCheck = new QCheckBox("Enable smooth animations");
    m_smoothAnimationsCheck->setChecked(DEFAULT_SMOOTH_ANIMATIONS);
    qualityLayout->addRow("", m_smoothAnimationsCheck);

    // Update frequency settings
    QGroupBox* updateGroup = new QGroupBox("Update Frequency");
    QFormLayout* updateLayout = new QFormLayout(updateGroup);

    m_updateFrequencySlider = new QSlider(Qt::Horizontal);
    m_updateFrequencySlider->setRange(30, 120);
    m_updateFrequencySlider->setValue(DEFAULT_UPDATE_FREQUENCY);
    m_updateFrequencyLabel = new QLabel(QString("%1 FPS").arg(DEFAULT_UPDATE_FREQUENCY));

    QHBoxLayout* updateSliderLayout = new QHBoxLayout();
    updateSliderLayout->addWidget(m_updateFrequencySlider);
    updateSliderLayout->addWidget(m_updateFrequencyLabel);
    updateLayout->addRow("Target FPS:", updateSliderLayout);

    layout->addWidget(qualityGroup);
    layout->addWidget(updateGroup);
    layout->addStretch();

    m_tabWidget->addTab(m_performanceTab, "Performance");
}

void SettingsDialog::createDisplayTab()
{
    m_displayTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_displayTab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    // Grid settings
    QGroupBox* gridGroup = new QGroupBox("Grid Display");
    QFormLayout* gridLayout = new QFormLayout(gridGroup);

    m_gridOpacitySlider = new QSlider(Qt::Horizontal);
    m_gridOpacitySlider->setRange(10, 100);
    m_gridOpacitySlider->setValue(DEFAULT_GRID_OPACITY);
    m_gridOpacityLabel = new QLabel(QString("%1%").arg(DEFAULT_GRID_OPACITY));

    QHBoxLayout* gridOpacityLayout = new QHBoxLayout();
    gridOpacityLayout->addWidget(m_gridOpacitySlider);
    gridOpacityLayout->addWidget(m_gridOpacityLabel);
    gridLayout->addRow("Grid Opacity:", gridOpacityLayout);

    m_gridColorButton = new QPushButton();
    m_gridColorButton->setFixedSize(50, 30);
    updateColorButton(m_gridColorButton, m_gridColor);
    gridLayout->addRow("Grid Color:", m_gridColorButton);

    // Fog brush settings
    QGroupBox* brushGroup = new QGroupBox("Default Fog Brush");
    QFormLayout* brushLayout = new QFormLayout(brushGroup);

    m_defaultFogBrushSlider = new QSlider(Qt::Horizontal);
    m_defaultFogBrushSlider->setRange(10, 200);
    m_defaultFogBrushSlider->setValue(DEFAULT_FOG_BRUSH_SIZE);
    m_defaultFogBrushLabel = new QLabel(QString("%1 pixels").arg(DEFAULT_FOG_BRUSH_SIZE));

    QHBoxLayout* brushSliderLayout = new QHBoxLayout();
    brushSliderLayout->addWidget(m_defaultFogBrushSlider);
    brushSliderLayout->addWidget(m_defaultFogBrushLabel);
    brushLayout->addRow("Brush Size:", brushSliderLayout);

    // Interaction settings
    QGroupBox* interactionGroup = new QGroupBox("Interaction");
    QFormLayout* interactionLayout = new QFormLayout(interactionGroup);
    m_wheelZoomCheck = new QCheckBox("Enable mouse wheel/touchpad zoom");
    m_wheelZoomCheck->setChecked(DEFAULT_WHEEL_ZOOM_ENABLED);
    interactionLayout->addRow("Wheel Zoom:", m_wheelZoomCheck);

    layout->addWidget(gridGroup);
    layout->addWidget(brushGroup);
    layout->addWidget(interactionGroup);
    layout->addStretch();

    m_tabWidget->addTab(m_displayTab, "Display");
}

void SettingsDialog::createButtonBox()
{
    m_buttonBox = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(m_buttonBox);
    buttonLayout->setContentsMargins(0, 8, 0, 0);

    m_resetButton = new QPushButton("Reset to Defaults");
    m_cancelButton = new QPushButton("Cancel");
    m_applyButton = new QPushButton("Apply");
    m_okButton = new QPushButton("OK");

    // Set primary button style
    m_okButton->setProperty("primary", true);
    m_applyButton->setProperty("primary", true);

    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_okButton);
}

void SettingsDialog::connectSignals()
{
    // GM Beacon signals
    connect(m_gmBeaconSizeSlider, &QSlider::valueChanged, this, &SettingsDialog::onGMBeaconSizeChanged);
    connect(m_gmBeaconColorButton, &QPushButton::clicked, this, &SettingsDialog::onGMBeaconColorClicked);
    connect(m_gmBeaconShapeGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &SettingsDialog::onGMBeaconShapeChanged);
    connect(m_gmBeaconOpacitySlider, &QSlider::valueChanged, this, &SettingsDialog::onGMBeaconOpacityChanged);

    // Fog/Mist signals
    connect(m_fogAnimationSpeedSlider, &QSlider::valueChanged, this, &SettingsDialog::onFogAnimationSpeedChanged);
    connect(m_fogOpacitySlider, &QSlider::valueChanged, this, &SettingsDialog::onFogOpacityChanged);
    connect(m_fogTextureCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onFogTextureChanged);

    // Performance signals
    connect(m_animationQualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onAnimationQualityChanged);
    connect(m_smoothAnimationsCheck, &QCheckBox::toggled, this, &SettingsDialog::onSmoothAnimationsToggled);
    connect(m_updateFrequencySlider, &QSlider::valueChanged, this, &SettingsDialog::onUpdateFrequencyChanged);

    // Display signals
    connect(m_gridOpacitySlider, &QSlider::valueChanged, this, &SettingsDialog::onGridOpacityChanged);
    connect(m_gridColorButton, &QPushButton::clicked, this, &SettingsDialog::onGridColorClicked);
    connect(m_defaultFogBrushSlider, &QSlider::valueChanged, this, &SettingsDialog::onDefaultFogBrushSizeChanged);
    connect(m_wheelZoomCheck, &QCheckBox::toggled, this, [this](bool enabled){ m_settings.wheelZoomEnabled = enabled; });

    // Button signals
    connect(m_okButton, &QPushButton::clicked, this, [this]() {
        saveSettings();
        applySettings();
        accept();
    });

    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(m_applyButton, &QPushButton::clicked, this, [this]() {
        saveSettings();
        applySettings();
    });

    connect(m_resetButton, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
}

void SettingsDialog::onGMBeaconSizeChanged(int value)
{
    m_settings.gmBeaconSize = value;
    m_gmBeaconSizeLabel->setText(QString("%1% of viewport").arg(value));
}

void SettingsDialog::onGMBeaconColorClicked()
{
    if (!m_colorDialog) {
        m_colorDialog = new QColorDialog(this);
    }

    m_colorDialog->setCurrentColor(m_gmBeaconColor);
    if (m_colorDialog->exec() == QDialog::Accepted) {
        m_gmBeaconColor = m_colorDialog->currentColor();
        m_settings.gmBeaconColor = m_gmBeaconColor;
        updateColorButton(m_gmBeaconColorButton, m_gmBeaconColor);
    }
}

void SettingsDialog::onGMBeaconShapeChanged(QAbstractButton* button)
{
    Q_UNUSED(button)
    m_settings.gmBeaconShape = m_gmBeaconShapeGroup->checkedId();
}

void SettingsDialog::onGMBeaconOpacityChanged(int value)
{
    m_settings.gmBeaconOpacity = value;
    m_gmBeaconOpacityLabel->setText(QString("%1%").arg(value));
}

void SettingsDialog::onFogAnimationSpeedChanged(int value)
{
    m_settings.fogAnimationSpeed = value;
    m_fogAnimationSpeedLabel->setText(QString("%1%").arg(value));
}

void SettingsDialog::onFogOpacityChanged(int value)
{
    m_settings.fogOpacity = value;
    m_fogOpacityLabel->setText(QString("%1%").arg(value));
}

void SettingsDialog::onFogTextureChanged(int index)
{
    m_settings.fogTextureIndex = index;
}

void SettingsDialog::onAnimationQualityChanged(int index)
{
    m_settings.animationQuality = index;
}

void SettingsDialog::onSmoothAnimationsToggled(bool enabled)
{
    m_settings.smoothAnimations = enabled;
}

void SettingsDialog::onUpdateFrequencyChanged(int value)
{
    m_settings.updateFrequency = value;
    m_updateFrequencyLabel->setText(QString("%1 FPS").arg(value));
}

void SettingsDialog::onGridOpacityChanged(int value)
{
    m_settings.gridOpacity = value;
    m_gridOpacityLabel->setText(QString("%1%").arg(value));
}

void SettingsDialog::onGridColorClicked()
{
    if (!m_colorDialog) {
        m_colorDialog = new QColorDialog(this);
    }

    m_colorDialog->setCurrentColor(m_gridColor);
    if (m_colorDialog->exec() == QDialog::Accepted) {
        m_gridColor = m_colorDialog->currentColor();
        m_settings.gridColor = m_gridColor;
        updateColorButton(m_gridColorButton, m_gridColor);
    }
}

void SettingsDialog::onDefaultFogBrushSizeChanged(int value)
{
    m_settings.defaultFogBrushSize = value;
    m_defaultFogBrushLabel->setText(QString("%1 pixels").arg(value));
}

void SettingsDialog::resetToDefaults()
{
    // Reset all controls to default values
    m_gmBeaconSizeSlider->setValue(DEFAULT_GM_BEACON_SIZE);
    m_gmBeaconColor = DEFAULT_GM_BEACON_COLOR;
    updateColorButton(m_gmBeaconColorButton, m_gmBeaconColor);
    m_circleShapeRadio->setChecked(true);
    m_gmBeaconOpacitySlider->setValue(DEFAULT_GM_BEACON_OPACITY);

    m_fogAnimationSpeedSlider->setValue(DEFAULT_FOG_ANIMATION_SPEED);
    m_fogOpacitySlider->setValue(DEFAULT_FOG_OPACITY);
    m_fogTextureCombo->setCurrentIndex(DEFAULT_FOG_TEXTURE);

    m_animationQualityCombo->setCurrentIndex(DEFAULT_ANIMATION_QUALITY);
    m_smoothAnimationsCheck->setChecked(DEFAULT_SMOOTH_ANIMATIONS);
    m_updateFrequencySlider->setValue(DEFAULT_UPDATE_FREQUENCY);

    m_gridOpacitySlider->setValue(DEFAULT_GRID_OPACITY);
    m_gridColor = DEFAULT_GRID_COLOR;
    updateColorButton(m_gridColorButton, m_gridColor);
    m_defaultFogBrushSlider->setValue(DEFAULT_FOG_BRUSH_SIZE);
    m_wheelZoomCheck->setChecked(DEFAULT_WHEEL_ZOOM_ENABLED);

    // Update settings struct
    m_settings.gmBeaconSize = DEFAULT_GM_BEACON_SIZE;
    m_settings.gmBeaconColor = DEFAULT_GM_BEACON_COLOR;
    m_settings.gmBeaconShape = DEFAULT_GM_BEACON_SHAPE;
    m_settings.gmBeaconOpacity = DEFAULT_GM_BEACON_OPACITY;
    m_settings.fogAnimationSpeed = DEFAULT_FOG_ANIMATION_SPEED;
    m_settings.fogOpacity = DEFAULT_FOG_OPACITY;
    m_settings.fogTextureIndex = DEFAULT_FOG_TEXTURE;
    m_settings.animationQuality = DEFAULT_ANIMATION_QUALITY;
    m_settings.smoothAnimations = DEFAULT_SMOOTH_ANIMATIONS;
    m_settings.updateFrequency = DEFAULT_UPDATE_FREQUENCY;
    m_settings.gridOpacity = DEFAULT_GRID_OPACITY;
    m_settings.gridColor = DEFAULT_GRID_COLOR;
    m_settings.defaultFogBrushSize = DEFAULT_FOG_BRUSH_SIZE;
    m_settings.wheelZoomEnabled = DEFAULT_WHEEL_ZOOM_ENABLED;
}

void SettingsDialog::loadSettings()
{
    SettingsManager& settings = SettingsManager::instance();

    // Load GM Beacon settings
    m_settings.gmBeaconSize = settings.loadGMBeaconSize();
    m_settings.gmBeaconColor = settings.loadGMBeaconColor();
    m_settings.gmBeaconShape = settings.loadGMBeaconShape();
    m_settings.gmBeaconOpacity = settings.loadGMBeaconOpacity();

    // Load Fog/Mist settings
    m_settings.fogAnimationSpeed = settings.loadFogAnimationSpeed();
    m_settings.fogOpacity = settings.loadFogOpacity();
    m_settings.fogTextureIndex = settings.loadFogTextureIndex();

    // Load Performance settings
    m_settings.animationQuality = settings.loadAnimationQuality();
    m_settings.smoothAnimations = settings.loadSmoothAnimations();
    m_settings.updateFrequency = settings.loadUpdateFrequency();

    // Load Display settings
    m_settings.gridOpacity = settings.loadGridOpacity();
    m_settings.gridColor = settings.loadGridColor();
    m_settings.defaultFogBrushSize = settings.loadDefaultFogBrushSize();
    m_settings.wheelZoomEnabled = settings.loadWheelZoomEnabled();

    // Update UI controls
    m_gmBeaconSizeSlider->setValue(m_settings.gmBeaconSize);
    m_gmBeaconColor = m_settings.gmBeaconColor;
    updateColorButton(m_gmBeaconColorButton, m_gmBeaconColor);
    m_gmBeaconShapeGroup->button(m_settings.gmBeaconShape)->setChecked(true);
    m_gmBeaconOpacitySlider->setValue(m_settings.gmBeaconOpacity);

    m_fogAnimationSpeedSlider->setValue(m_settings.fogAnimationSpeed);
    m_fogOpacitySlider->setValue(m_settings.fogOpacity);
    m_fogTextureCombo->setCurrentIndex(m_settings.fogTextureIndex);

    m_animationQualityCombo->setCurrentIndex(m_settings.animationQuality);
    m_smoothAnimationsCheck->setChecked(m_settings.smoothAnimations);
    m_updateFrequencySlider->setValue(m_settings.updateFrequency);

    m_gridOpacitySlider->setValue(m_settings.gridOpacity);
    m_gridColor = m_settings.gridColor;
    updateColorButton(m_gridColorButton, m_gridColor);
    m_defaultFogBrushSlider->setValue(m_settings.defaultFogBrushSize);
    m_wheelZoomCheck->setChecked(m_settings.wheelZoomEnabled);
}

void SettingsDialog::saveSettings()
{
    SettingsManager& settings = SettingsManager::instance();

    // Save GM Beacon settings
    settings.saveGMBeaconSize(m_settings.gmBeaconSize);
    settings.saveGMBeaconColor(m_settings.gmBeaconColor);
    settings.saveGMBeaconShape(m_settings.gmBeaconShape);
    settings.saveGMBeaconOpacity(m_settings.gmBeaconOpacity);

    // Save Fog/Mist settings
    settings.saveFogAnimationSpeed(m_settings.fogAnimationSpeed);
    settings.saveFogOpacity(m_settings.fogOpacity);
    settings.saveFogTextureIndex(m_settings.fogTextureIndex);

    // Save Performance settings
    settings.saveAnimationQuality(m_settings.animationQuality);
    settings.saveSmoothAnimations(m_settings.smoothAnimations);
    settings.saveUpdateFrequency(m_settings.updateFrequency);

    // Save Display settings
    settings.saveGridOpacity(m_settings.gridOpacity);
    settings.saveGridColor(m_settings.gridColor);
    settings.saveDefaultFogBrushSize(m_settings.defaultFogBrushSize);
    settings.saveWheelZoomEnabled(m_settings.wheelZoomEnabled);
}

void SettingsDialog::applySettings()
{
    // This method will be called to apply settings to the main application
    // The actual implementation will depend on how the main application
    // wants to receive these settings updates

    // For now, we'll emit signals or use a callback system
    // This will be implemented based on MainWindow integration needs
}

void SettingsDialog::updateColorButton(QPushButton* button, const QColor& color)
{
    if (!button) return;

    QString styleSheet = QString(
        "QPushButton { "
        "background-color: %1; "
        "border: 2px solid %2; "
        "border-radius: 4px; "
        "} "
        "QPushButton:hover { "
        "border-color: %3; "
        "}"
    ).arg(color.name())
     .arg("#666")
     .arg("#888");

    button->setStyleSheet(styleSheet);
}
