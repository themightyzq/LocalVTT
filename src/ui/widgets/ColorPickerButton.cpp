#include "ColorPickerButton.h"
#include <QColorDialog>

ColorPickerButton::ColorPickerButton(QWidget* parent)
    : QPushButton(parent)
    , m_color(Qt::white)
{
    setFixedSize(32, 24);
    setCursor(Qt::PointingHandCursor);
    connect(this, &QPushButton::clicked, this, &ColorPickerButton::openColorDialog);
    updateButtonStyle();
}

ColorPickerButton::ColorPickerButton(const QColor& initialColor, QWidget* parent)
    : QPushButton(parent)
    , m_color(initialColor)
{
    setFixedSize(32, 24);
    setCursor(Qt::PointingHandCursor);
    connect(this, &QPushButton::clicked, this, &ColorPickerButton::openColorDialog);
    updateButtonStyle();
}

void ColorPickerButton::setColor(const QColor& color)
{
    if (m_color != color) {
        m_color = color;
        updateButtonStyle();
        // Don't emit colorChanged here - only emit when user selects via dialog
    }
}

void ColorPickerButton::openColorDialog()
{
    QColorDialog::ColorDialogOptions options;
    if (m_alphaEnabled) {
        options |= QColorDialog::ShowAlphaChannel;
    }

    QColor newColor = QColorDialog::getColor(m_color, this, tr("Select Color"), options);

    if (newColor.isValid() && newColor != m_color) {
        m_color = newColor;
        updateButtonStyle();
        emit colorChanged(m_color);
    }
}

void ColorPickerButton::updateButtonStyle()
{
    // Create a style with the color swatch and dark theme border
    QString colorHex = m_color.name(QColor::HexArgb);

    // For display, use opaque version if alpha is very low
    QColor displayColor = m_color;
    if (displayColor.alpha() < 50) {
        displayColor.setAlpha(255);
    }

    setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid #3A3A3A;
            border-radius: 3px;
        }
        QPushButton:hover {
            border: 1px solid #4A90E2;
        }
        QPushButton:pressed {
            border: 2px solid #4A90E2;
        }
    )").arg(displayColor.name()));

    // Set tooltip showing color info
    QString tooltip = QString("<b>Color</b><br>%1").arg(m_color.name(QColor::HexRgb).toUpper());
    if (m_alphaEnabled && m_color.alpha() < 255) {
        tooltip += QString("<br>Alpha: %1%").arg(qRound(m_color.alphaF() * 100));
    }
    setToolTip(tooltip);
}
