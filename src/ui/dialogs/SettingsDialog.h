#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QColor>

class QTabWidget;
class QWidget;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QSlider;
class QLabel;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QColorDialog;
class QButtonGroup;
class QRadioButton;
class QAbstractButton;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // Apply all settings to the application
    void applySettings();

private slots:
    void onGMBeaconSizeChanged(int value);
    void onGMBeaconColorClicked();
    void onGMBeaconShapeChanged(QAbstractButton* button);
    void onGMBeaconOpacityChanged(int value);

    void onFogAnimationSpeedChanged(int value);
    void onFogOpacityChanged(int value);
    void onFogTextureChanged(int index);

    void onAnimationQualityChanged(int index);
    void onSmoothAnimationsToggled(bool enabled);
    void onUpdateFrequencyChanged(int value);

    void onGridOpacityChanged(int value);
    void onGridColorClicked();
    void onDefaultFogBrushSizeChanged(int value);

    void resetToDefaults();
    void loadSettings();
    void saveSettings();

private:
    void setupUI();
    void createGMBeaconTab();
    void createFogMistTab();
    void createPerformanceTab();
    void createDisplayTab();
    void createButtonBox();

    void updateColorButton(QPushButton* button, const QColor& color);
    void connectSignals();

    // Main layout
    QTabWidget* m_tabWidget;
    QVBoxLayout* m_mainLayout;

    // GM Beacon Settings
    QWidget* m_gmBeaconTab;
    QSlider* m_gmBeaconSizeSlider;
    QLabel* m_gmBeaconSizeLabel;
    QPushButton* m_gmBeaconColorButton;
    QColor m_gmBeaconColor;
    QButtonGroup* m_gmBeaconShapeGroup;
    QRadioButton* m_circleShapeRadio;
    QRadioButton* m_starShapeRadio;
    QRadioButton* m_crosshairShapeRadio;
    QSlider* m_gmBeaconOpacitySlider;
    QLabel* m_gmBeaconOpacityLabel;

    // Fog/Mist Settings
    QWidget* m_fogMistTab;
    QSlider* m_fogAnimationSpeedSlider;
    QLabel* m_fogAnimationSpeedLabel;
    QSlider* m_fogOpacitySlider;
    QLabel* m_fogOpacityLabel;
    QComboBox* m_fogTextureCombo;

    // Performance Settings
    QWidget* m_performanceTab;
    QComboBox* m_animationQualityCombo;
    QCheckBox* m_smoothAnimationsCheck;
    QSlider* m_updateFrequencySlider;
    QLabel* m_updateFrequencyLabel;

    // Display Settings
    QWidget* m_displayTab;
    QSlider* m_gridOpacitySlider;
    QLabel* m_gridOpacityLabel;
    QPushButton* m_gridColorButton;
    QColor m_gridColor;
    QSlider* m_defaultFogBrushSlider;
    QLabel* m_defaultFogBrushLabel;
    QCheckBox* m_wheelZoomCheck;

    // Dialog buttons
    QWidget* m_buttonBox;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;

    // Color dialog
    QColorDialog* m_colorDialog;

    // Settings values
    struct Settings {
        // GM Beacon
        int gmBeaconSize;
        QColor gmBeaconColor;
        int gmBeaconShape; // 0=circle, 1=star, 2=crosshair
        int gmBeaconOpacity;

        // Fog/Mist
        int fogAnimationSpeed;
        int fogOpacity;
        int fogTextureIndex;

        // Performance
        int animationQuality; // 0=low, 1=medium, 2=high
        bool smoothAnimations;
        int updateFrequency;

        // Display
        int gridOpacity;
        QColor gridColor;
        int defaultFogBrushSize;
        bool wheelZoomEnabled;
    } m_settings;

    // Default values
    static const int DEFAULT_GM_BEACON_SIZE = 20;
    static const QColor DEFAULT_GM_BEACON_COLOR;
    static const int DEFAULT_GM_BEACON_SHAPE = 0; // circle
    static const int DEFAULT_GM_BEACON_OPACITY = 90;

    static const int DEFAULT_FOG_ANIMATION_SPEED = 50;
    static const int DEFAULT_FOG_OPACITY = 80;
    static const int DEFAULT_FOG_TEXTURE = 0;

    static const int DEFAULT_ANIMATION_QUALITY = 1; // medium
    static const bool DEFAULT_SMOOTH_ANIMATIONS = true;
    static const int DEFAULT_UPDATE_FREQUENCY = 60;

    static const int DEFAULT_GRID_OPACITY = 50;
    static const QColor DEFAULT_GRID_COLOR;
    static const int DEFAULT_FOG_BRUSH_SIZE = 50;
    static const bool DEFAULT_WHEEL_ZOOM_ENABLED = false;
};

#endif // SETTINGSDIALOG_H
