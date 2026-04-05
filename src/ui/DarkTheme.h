#pragma once

#include <QString>
#include <QColor>

namespace DarkTheme {
    // --- Color palette ---
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

    // --- Typography scale ---
    constexpr int FontSmall  = 11;   // Labels, hints, secondary info
    constexpr int FontBase   = 12;   // Body text, controls, spinners
    constexpr int FontMedium = 14;   // Section headers, prominent buttons
    constexpr int FontLarge  = 18;   // Dialog titles, empty state

    // --- Spacing scale ---
    constexpr int SpaceTight   = 4;
    constexpr int SpaceBase    = 8;
    constexpr int SpaceLoose   = 12;
    constexpr int SpaceSection = 16;

    // --- Corner radii ---
    constexpr int RadiusSmall = 4;
    constexpr int RadiusBase  = 6;

    // --- Helpers ---
    inline QString hex(const QColor& c) { return c.name(); }
    inline QString rgba(const QColor& c, double opacity) {
        return QString("rgba(%1,%2,%3,%4)")
            .arg(c.red()).arg(c.green()).arg(c.blue()).arg(opacity);
    }

    // --- Component stylesheet factories ---
    QString toolButtonStyle();       // Standard toolbar button
    QString dangerButtonStyle();     // Destructive action (red tint)
    QString successButtonStyle();    // Positive action (green tint)
    QString spinBoxStyle();          // Number spinners
    QString toolBarStyle();          // Toolbar container

    // --- Existing factories ---
    QString getApplicationStyleSheet();
    QString getMainWindowStyleSheet();
    QString getToolboxStyleSheet();
    QString getButtonStyleSheet();
    QString getMenuStyleSheet();
    QString getSliderStyleSheet();
    QString getTabStyleSheet();
    QString getScrollBarStyleSheet();
}