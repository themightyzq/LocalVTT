#include "ui/DarkTheme.h"

namespace DarkTheme {

QString getApplicationStyleSheet()
{
    return R"(
        QWidget {
            background-color: #1a1a1a;
            color: #E0E0E0;
            font-size: 14px;
        }

        QMainWindow {
            background-color: #1a1a1a;
        }

        QDockWidget {
            background-color: #242424;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
        }

        QDockWidget::title {
            background-color: #2d2d2d;
            padding: 8px;
            border-bottom: 1px solid #3A3A3A;
        }

        QToolBar {
            background-color: #242424;
            border: none;
            spacing: 4px;
            padding: 4px;
        }

        QStatusBar {
            background-color: #242424;
            border-top: 1px solid #3A3A3A;
        }

        QGraphicsView {
            background-color: #1a1a1a;
            border: 1px solid #3A3A3A;
        }
    )";
}

QString getMainWindowStyleSheet()
{
    return getApplicationStyleSheet() + R"(
        /* Empty state styling */
        QLabel#emptyStateLabel {
            color: #808080;
            font-size: 24px;
            font-weight: 300;
            background-color: transparent;
        }
    )";
}

QString getToolboxStyleSheet()
{
    return R"(
        QGroupBox {
            background-color: #242424;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 8px;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px 0 4px;
            color: #A0A0A0;
            font-size: 12px;
            font-weight: bold;
        }

        QLabel {
            color: #E0E0E0;
            background-color: transparent;
        }

        QLabel:disabled {
            color: #808080;
        }
    )";
}

QString getButtonStyleSheet()
{
    return R"(
        QPushButton {
            background-color: #242424;
            color: #E0E0E0;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            padding: 6px 12px;
            min-height: 24px;
        }

        QPushButton:hover {
            background-color: #3A3A3A;
            border-color: #5BA3F5;
        }

        QPushButton:pressed {
            background-color: #1a1a1a;
        }

        QPushButton:checked {
            background-color: #4A90E2;
            color: white;
            border-color: #4A90E2;
        }

        QPushButton:disabled {
            background-color: #1a1a1a;
            color: #808080;
            border-color: #2d2d2d;
        }

        QPushButton#primaryButton {
            background-color: #4A90E2;
            color: white;
            border-color: #4A90E2;
            font-weight: bold;
        }

        QPushButton#primaryButton:hover {
            background-color: #5BA3F5;
        }

        QPushButton#dangerButton {
            background-color: #E74C3C;
            color: white;
            border-color: #E74C3C;
        }

        QPushButton#dangerButton:hover {
            background-color: #FF5C4C;
        }

        QToolButton {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 4px;
        }

        QToolButton:hover {
            background-color: #3A3A3A;
            border-color: #5BA3F5;
        }

        QToolButton:pressed {
            background-color: #1a1a1a;
        }

        QToolButton:checked {
            background-color: #4A90E2;
            color: white;
        }
    )";
}

QString getMenuStyleSheet()
{
    return R"(
        QMenuBar {
            background-color: #242424;
            border-bottom: 1px solid #3A3A3A;
        }

        QMenuBar::item {
            background-color: transparent;
            color: #E0E0E0;
            padding: 4px 8px;
        }

        QMenuBar::item:selected {
            background-color: #2d2d2d;
        }

        QMenu {
            background-color: #242424;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            padding: 4px 0;
        }

        QMenu::item {
            background-color: transparent;
            color: #E0E0E0;
            padding: 6px 20px;
        }

        QMenu::item:selected {
            background-color: #4A90E2;
            color: white;
        }

        QMenu::item:disabled {
            color: #808080;
        }

        QMenu::separator {
            height: 1px;
            background-color: #3A3A3A;
            margin: 4px 10px;
        }
    )";
}

QString getSliderStyleSheet()
{
    return R"(
        QSlider {
            background-color: transparent;
        }

        QSlider::groove:horizontal {
            background-color: #3A3A3A;
            height: 4px;
            border-radius: 2px;
        }

        QSlider::handle:horizontal {
            background-color: #4A90E2;
            width: 16px;
            height: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }

        QSlider::handle:horizontal:hover {
            background-color: #3A7BC8;
        }

        QSlider::sub-page:horizontal {
            background-color: #4A90E2;
            border-radius: 2px;
        }

        QSpinBox, QDoubleSpinBox {
            background-color: #242424;
            color: #E0E0E0;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            padding: 4px;
        }

        QSpinBox:hover, QDoubleSpinBox:hover {
            border-color: #4A90E2;
        }

        QSpinBox::up-button, QDoubleSpinBox::up-button,
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            background-color: #2d2d2d;
            border: none;
            width: 16px;
        }

        QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
            background-color: #3A3A3A;
        }
    )";
}

QString getTabStyleSheet()
{
    return R"(
        QTabWidget::pane {
            background-color: #1a1a1a;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
        }

        QTabBar::tab {
            background-color: #242424;
            color: #A0A0A0;
            padding: 8px 16px;
            margin-right: 2px;
            border: 1px solid #3A3A3A;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }

        QTabBar::tab:selected {
            background-color: #1a1a1a;
            color: #E0E0E0;
            border-color: #4A90E2;
        }

        QTabBar::tab:hover:!selected {
            background-color: #2d2d2d;
        }
    )";
}

QString getScrollBarStyleSheet()
{
    return R"(
        QScrollBar:vertical {
            background-color: #1a1a1a;
            width: 12px;
            border: none;
        }

        QScrollBar::handle:vertical {
            background-color: #3A3A3A;
            border-radius: 6px;
            min-height: 20px;
        }

        QScrollBar::handle:vertical:hover {
            background-color: #4A4A4A;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
            height: 0;
        }

        QScrollBar:horizontal {
            background-color: #1a1a1a;
            height: 12px;
            border: none;
        }

        QScrollBar::handle:horizontal {
            background-color: #3A3A3A;
            border-radius: 6px;
            min-width: 20px;
        }

        QScrollBar::handle:horizontal:hover {
            background-color: #4A4A4A;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            border: none;
            background: none;
            width: 0;
        }

        QComboBox {
            background-color: #242424;
            color: #E0E0E0;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            padding: 4px 8px;
            min-height: 24px;
        }

        QComboBox:hover {
            border-color: #4A90E2;
        }

        QComboBox::drop-down {
            border: none;
            width: 20px;
        }

        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #E0E0E0;
            width: 0;
            height: 0;
            margin-right: 4px;
        }

        QComboBox QAbstractItemView {
            background-color: #242424;
            border: 1px solid #3A3A3A;
            selection-background-color: #4A90E2;
            selection-color: white;
        }

        QLineEdit {
            background-color: #242424;
            color: #E0E0E0;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            padding: 4px 8px;
        }

        QLineEdit:focus {
            border-color: #4A90E2;
        }

        QLineEdit:disabled {
            background-color: #1a1a1a;
            color: #808080;
        }
    )";
}

QString toolBarStyle()
{
    return QString(
        "QToolBar {"
        "  background: %1; border: none; spacing: %2px; padding: %3px;"
        "}"
        "QToolBar::separator {"
        "  background: %4; width: 1px; margin: 4px 8px;"
        "}"
    ).arg(hex(BgPrimary),
          QString::number(SpaceTight),
          QString::number(SpaceBase),
          rgba(TextPrimary, 0.08));
}

QString toolButtonStyle()
{
    return QString(
        "QToolButton {"
        "  background: %1; border: 1px solid %2; border-radius: %3px;"
        "  padding: 8px; min-width: 32px; min-height: 32px;"
        "  color: %4; font-size: %5px;"
        "}"
        "QToolButton:hover:enabled {"
        "  background: %6; border-color: %7;"
        "}"
        "QToolButton:pressed:enabled {"
        "  background: %8;"
        "}"
        "QToolButton:checked {"
        "  background: %9; border-color: %10; color: white;"
        "}"
        "QToolButton:disabled {"
        "  background: %11; border-color: %12; color: %13;"
        "}"
        "QToolButton:focus {"
        "  outline: 2px solid %10; outline-offset: 2px;"
        "}"
    ).arg(rgba(TextPrimary, 0.05),    // 1: bg
          rgba(TextPrimary, 0.1),      // 2: border
          QString::number(RadiusBase), // 3: radius
          hex(TextPrimary),            // 4: text
          QString::number(FontBase),   // 5: font
          rgba(AccentPrimary, 0.15),   // 6: hover bg
          rgba(AccentPrimary, 0.3),    // 7: hover border
          rgba(AccentPrimary, 0.25),   // 8: pressed bg
          rgba(AccentPrimary, 0.3))    // 9: checked bg
     .arg(hex(AccentPrimary),          // 10: checked border / focus
          rgba(TextPrimary, 0.02),     // 11: disabled bg
          rgba(TextPrimary, 0.05),     // 12: disabled border
          rgba(TextPrimary, 0.3));     // 13: disabled text
}

QString dangerButtonStyle()
{
    return QString(
        "QToolButton {"
        "  background: %1; border: 1px solid %2; border-radius: %3px;"
        "  padding: 8px; min-width: 32px; min-height: 32px;"
        "  color: %4; font-size: %5px;"
        "}"
        "QToolButton:hover:enabled {"
        "  background: %6; border-color: %7;"
        "}"
        "QToolButton:disabled {"
        "  background: %8; border-color: %9; color: %10;"
        "}"
    ).arg(rgba(AccentDanger, 0.15),    // 1: bg
          rgba(AccentDanger, 0.3),     // 2: border
          QString::number(RadiusBase), // 3: radius
          "#ff6b6b",                   // 4: text (bright red)
          QString::number(FontBase),   // 5: font
          rgba(AccentDanger, 0.3),     // 6: hover bg
          rgba(AccentDanger, 0.5),     // 7: hover border
          rgba(TextPrimary, 0.02),     // 8: disabled bg
          rgba(TextPrimary, 0.05),     // 9: disabled border
          rgba(TextPrimary, 0.3));     // 10: disabled text
}

QString successButtonStyle()
{
    return QString(
        "QToolButton {"
        "  background: %1; border: 1px solid %2; border-radius: %3px;"
        "  padding: 8px; min-width: 32px; min-height: 32px;"
        "  color: %4; font-size: %5px; font-weight: bold;"
        "}"
        "QToolButton:hover:enabled {"
        "  background: %6; border-color: %7;"
        "}"
        "QToolButton:checked {"
        "  background: %6; border-color: %7; color: %4;"
        "}"
        "QToolButton:disabled {"
        "  background: %8; border-color: %9; color: %10;"
        "}"
    ).arg(rgba(AccentSuccess, 0.3),    // 1: bg
          rgba(AccentSuccess, 0.5),    // 2: border
          QString::number(RadiusBase), // 3: radius
          "#6bffb8",                   // 4: text (bright green)
          QString::number(FontBase),   // 5: font
          rgba(AccentSuccess, 0.4),    // 6: hover/checked bg
          rgba(AccentSuccess, 0.6),    // 7: hover/checked border
          rgba(TextPrimary, 0.02),     // 8: disabled bg
          rgba(TextPrimary, 0.05),     // 9: disabled border
          rgba(TextPrimary, 0.3));     // 10: disabled text
}

QString spinBoxStyle()
{
    return QString(
        "QSpinBox {"
        "  background: %1; border: 1px solid %2; border-radius: %3px;"
        "  padding: 4px 8px; color: %4; font-size: %5px;"
        "  min-width: 60px; max-width: 85px;"
        "}"
        "QSpinBox:hover {"
        "  background: %6; border-color: %7;"
        "}"
        "QSpinBox:focus {"
        "  border-color: %8;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "  background: %1; border: 1px solid %2; width: 20px;"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover {"
        "  background: %6;"
        "}"
        "QSpinBox:disabled {"
        "  background: %9; color: %10;"
        "}"
    ).arg(rgba(TextPrimary, 0.05),    // 1: bg
          rgba(TextPrimary, 0.1),      // 2: border
          QString::number(RadiusSmall),// 3: radius
          hex(TextPrimary),            // 4: text
          QString::number(FontBase),   // 5: font
          rgba(TextPrimary, 0.08),     // 6: hover bg
          rgba(AccentPrimary, 0.3),    // 7: hover border
          hex(AccentPrimary),          // 8: focus border
          hex(BgPrimary),              // 9: disabled bg
          hex(TextDisabled));          // 10: disabled text
}

}