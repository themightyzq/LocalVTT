#pragma once

#include <QPushButton>
#include <QColor>

/**
 * @brief A button that displays a color swatch and opens a color picker dialog when clicked.
 *
 * Used in the Atmosphere Toolbox for selecting fog tint, lighting tint, etc.
 */
class ColorPickerButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ColorPickerButton(QWidget* parent = nullptr);
    explicit ColorPickerButton(const QColor& initialColor, QWidget* parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor& color);

    // Whether to include alpha channel in the color picker
    void setAlphaEnabled(bool enabled) { m_alphaEnabled = enabled; }
    bool isAlphaEnabled() const { return m_alphaEnabled; }

signals:
    void colorChanged(const QColor& color);

private slots:
    void openColorDialog();

private:
    void updateButtonStyle();

    QColor m_color;
    bool m_alphaEnabled = false;
};
