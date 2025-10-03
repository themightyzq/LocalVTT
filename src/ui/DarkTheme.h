#pragma once

#include <QString>
#include <QColor>

namespace DarkTheme {
    inline const QColor BgPrimary{"#1a1a1a"};      // Main background
    inline const QColor BgSecondary{"#242424"};     // Panels
    inline const QColor BgTertiary{"#2d2d2d"};      // Hover states
    inline const QColor AccentPrimary{"#4A90E2"};   // Primary actions
    inline const QColor AccentDanger{"#E74C3C"};    // Warnings/Blackout
    inline const QColor AccentSuccess{"#27AE60"};   // Success states
    inline const QColor AccentWarning{"#F39C12"};   // Warning states
    inline const QColor TextPrimary{"#E0E0E0"};     // Main text
    inline const QColor TextSecondary{"#A0A0A0"};   // Secondary text
    inline const QColor TextDisabled{"#606060"};    // Disabled text
    inline const QColor BorderColor{"#3A3A3A"};     // Borders
    inline const QColor GridColor{"#4A90E233"};     // 20% opacity blue

    QString getApplicationStyleSheet();
    QString getMainWindowStyleSheet();
    QString getToolboxStyleSheet();
    QString getButtonStyleSheet();
    QString getMenuStyleSheet();
    QString getSliderStyleSheet();
    QString getTabStyleSheet();
    QString getScrollBarStyleSheet();
}