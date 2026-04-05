#ifndef LIGHTEDITDIALOG_H
#define LIGHTEDITDIALOG_H

#include <QDialog>
#include <QColor>
#include "graphics/PointLight.h"

class QLineEdit;
class QSlider;
class QLabel;
class QPushButton;
class QCheckBox;
class QDoubleSpinBox;

// Dialog for editing point light properties
// Opened when double-clicking a light in Light Placement Mode
class LightEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LightEditDialog(QWidget* parent = nullptr);
    ~LightEditDialog() override;

    // Set the light to edit (copies values into dialog)
    void setLight(const PointLight& light);

    // Get the edited light data
    PointLight getLight() const;

private slots:
    void onColorButtonClicked();
    void onFlickerToggled(bool enabled);
    void onRadiusChanged(int value);
    void onIntensityChanged(int value);
    void onFalloffChanged(int value);
    void onFlickerAmountChanged(int value);

private:
    void setupUI();
    void updateColorButton();
    void updateLabels();

    // Light being edited
    PointLight m_light;

    // UI Controls
    QLineEdit* m_nameEdit;
    QPushButton* m_colorButton;
    QSlider* m_radiusSlider;
    QLabel* m_radiusLabel;
    QDoubleSpinBox* m_radiusSpin;
    QSlider* m_intensitySlider;
    QLabel* m_intensityLabel;
    QSlider* m_falloffSlider;
    QLabel* m_falloffLabel;
    QCheckBox* m_flickerCheck;
    QSlider* m_flickerAmountSlider;
    QLabel* m_flickerAmountLabel;

    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Color storage
    QColor m_currentColor;
};

#endif // LIGHTEDITDIALOG_H
