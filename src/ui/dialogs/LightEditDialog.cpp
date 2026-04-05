#include "LightEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QColorDialog>

LightEditDialog::LightEditDialog(QWidget* parent)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_colorButton(nullptr)
    , m_radiusSlider(nullptr)
    , m_radiusLabel(nullptr)
    , m_radiusSpin(nullptr)
    , m_intensitySlider(nullptr)
    , m_intensityLabel(nullptr)
    , m_falloffSlider(nullptr)
    , m_falloffLabel(nullptr)
    , m_flickerCheck(nullptr)
    , m_flickerAmountSlider(nullptr)
    , m_flickerAmountLabel(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_currentColor(255, 200, 100)
{
    setWindowTitle("Edit Light");
    setModal(true);
    setMinimumWidth(350);
    setupUI();
}

LightEditDialog::~LightEditDialog()
{
    // Qt parent-child handles cleanup
}

void LightEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Name field
    QHBoxLayout* nameLayout = new QHBoxLayout();
    QLabel* nameLabel = new QLabel("Name:");
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Optional light name");
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);
    mainLayout->addLayout(nameLayout);

    // Color group
    QGroupBox* colorGroup = new QGroupBox("Color");
    QHBoxLayout* colorLayout = new QHBoxLayout(colorGroup);
    m_colorButton = new QPushButton();
    m_colorButton->setFixedSize(60, 30);
    m_colorButton->setToolTip("Click to change light color");
    connect(m_colorButton, &QPushButton::clicked, this, &LightEditDialog::onColorButtonClicked);
    colorLayout->addWidget(new QLabel("Light Color:"));
    colorLayout->addWidget(m_colorButton);
    colorLayout->addStretch();
    mainLayout->addWidget(colorGroup);

    // Properties group
    QGroupBox* propsGroup = new QGroupBox("Properties");
    QFormLayout* propsLayout = new QFormLayout(propsGroup);
    propsLayout->setSpacing(8);

    // Radius
    QHBoxLayout* radiusLayout = new QHBoxLayout();
    m_radiusSlider = new QSlider(Qt::Horizontal);
    m_radiusSlider->setRange(50, 500);
    m_radiusSlider->setValue(200);
    m_radiusSpin = new QDoubleSpinBox();
    m_radiusSpin->setRange(50, 500);
    m_radiusSpin->setValue(200);
    m_radiusSpin->setSuffix(" px");
    m_radiusSpin->setFixedWidth(80);
    connect(m_radiusSlider, &QSlider::valueChanged, this, &LightEditDialog::onRadiusChanged);
    connect(m_radiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double val) { m_radiusSlider->setValue(static_cast<int>(val)); });
    radiusLayout->addWidget(m_radiusSlider);
    radiusLayout->addWidget(m_radiusSpin);
    propsLayout->addRow("Radius:", radiusLayout);

    // Intensity
    QHBoxLayout* intensityLayout = new QHBoxLayout();
    m_intensitySlider = new QSlider(Qt::Horizontal);
    m_intensitySlider->setRange(0, 100);
    m_intensitySlider->setValue(100);
    m_intensityLabel = new QLabel("100%");
    m_intensityLabel->setFixedWidth(45);
    connect(m_intensitySlider, &QSlider::valueChanged, this, &LightEditDialog::onIntensityChanged);
    intensityLayout->addWidget(m_intensitySlider);
    intensityLayout->addWidget(m_intensityLabel);
    propsLayout->addRow("Intensity:", intensityLayout);

    // Falloff
    QHBoxLayout* falloffLayout = new QHBoxLayout();
    m_falloffSlider = new QSlider(Qt::Horizontal);
    m_falloffSlider->setRange(10, 30);  // 1.0 to 3.0
    m_falloffSlider->setValue(15);      // 1.5 default
    m_falloffLabel = new QLabel("1.5");
    m_falloffLabel->setFixedWidth(45);
    m_falloffSlider->setToolTip("1.0 = linear, 2.0+ = sharper edge");
    connect(m_falloffSlider, &QSlider::valueChanged, this, &LightEditDialog::onFalloffChanged);
    falloffLayout->addWidget(m_falloffSlider);
    falloffLayout->addWidget(m_falloffLabel);
    propsLayout->addRow("Falloff:", falloffLayout);

    mainLayout->addWidget(propsGroup);

    // Flicker group
    QGroupBox* flickerGroup = new QGroupBox("Animation");
    QVBoxLayout* flickerLayout = new QVBoxLayout(flickerGroup);

    m_flickerCheck = new QCheckBox("Enable flicker animation");
    connect(m_flickerCheck, &QCheckBox::toggled, this, &LightEditDialog::onFlickerToggled);
    flickerLayout->addWidget(m_flickerCheck);

    QHBoxLayout* flickerAmountLayout = new QHBoxLayout();
    m_flickerAmountSlider = new QSlider(Qt::Horizontal);
    m_flickerAmountSlider->setRange(0, 30);  // 0.0 to 0.3
    m_flickerAmountSlider->setValue(10);     // 0.1 default
    m_flickerAmountSlider->setEnabled(false);
    m_flickerAmountLabel = new QLabel("10%");
    m_flickerAmountLabel->setFixedWidth(45);
    connect(m_flickerAmountSlider, &QSlider::valueChanged, this, &LightEditDialog::onFlickerAmountChanged);
    flickerAmountLayout->addWidget(new QLabel("Amount:"));
    flickerAmountLayout->addWidget(m_flickerAmountSlider);
    flickerAmountLayout->addWidget(m_flickerAmountLabel);
    flickerLayout->addLayout(flickerAmountLayout);

    mainLayout->addWidget(flickerGroup);

    // Button box
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_cancelButton = new QPushButton("Cancel");
    m_okButton = new QPushButton("OK");
    m_okButton->setDefault(true);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    mainLayout->addLayout(buttonLayout);
}

void LightEditDialog::setLight(const PointLight& light)
{
    m_light = light;
    m_currentColor = light.color;

    // Populate UI from light data
    m_nameEdit->setText(light.name);
    updateColorButton();

    m_radiusSlider->setValue(static_cast<int>(light.radius));
    m_radiusSpin->setValue(light.radius);

    m_intensitySlider->setValue(static_cast<int>(light.intensity * 100));

    m_falloffSlider->setValue(static_cast<int>(light.falloff * 10));

    m_flickerCheck->setChecked(light.flickering);
    m_flickerAmountSlider->setEnabled(light.flickering);
    m_flickerAmountSlider->setValue(static_cast<int>(light.flickerAmount * 100));

    updateLabels();
}

PointLight LightEditDialog::getLight() const
{
    PointLight light = m_light;

    light.name = m_nameEdit->text();
    light.color = m_currentColor;
    light.radius = m_radiusSpin->value();
    light.intensity = m_intensitySlider->value() / 100.0;
    light.falloff = m_falloffSlider->value() / 10.0;
    light.flickering = m_flickerCheck->isChecked();
    light.flickerAmount = m_flickerAmountSlider->value() / 100.0;

    return light;
}

void LightEditDialog::onColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_currentColor, this, "Select Light Color");
    if (color.isValid()) {
        m_currentColor = color;
        updateColorButton();
    }
}

void LightEditDialog::onFlickerToggled(bool enabled)
{
    m_flickerAmountSlider->setEnabled(enabled);
}

void LightEditDialog::onRadiusChanged(int value)
{
    m_radiusSpin->setValue(value);
}

void LightEditDialog::onIntensityChanged(int value)
{
    m_intensityLabel->setText(QString("%1%").arg(value));
}

void LightEditDialog::onFalloffChanged(int value)
{
    m_falloffLabel->setText(QString::number(value / 10.0, 'f', 1));
}

void LightEditDialog::onFlickerAmountChanged(int value)
{
    m_flickerAmountLabel->setText(QString("%1%").arg(value));
}

void LightEditDialog::updateColorButton()
{
    QString style = QString(
        "background-color: %1; border: 1px solid #555; border-radius: 4px;"
    ).arg(m_currentColor.name());
    m_colorButton->setStyleSheet(style);
}

void LightEditDialog::updateLabels()
{
    onIntensityChanged(m_intensitySlider->value());
    onFalloffChanged(m_falloffSlider->value());
    onFlickerAmountChanged(m_flickerAmountSlider->value());
}
