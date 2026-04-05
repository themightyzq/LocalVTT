#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include <QPointF>
#include <QColor>
#include <QString>
#include <QUuid>

// Individual point light data
struct PointLight
{
    QUuid id;               // Unique identifier
    QPointF position;       // Scene coordinates
    QColor color;           // Light color (default warm yellow)
    qreal radius;           // Light radius in scene units
    qreal intensity;        // 0.0 to 1.0
    qreal falloff;          // Falloff curve (1.0 = linear, 2.0 = quadratic)
    bool enabled;           // Can be toggled on/off
    QString name;           // Optional label (e.g., "Torch 1")
    bool flickering;        // Animated flicker effect
    qreal flickerAmount;    // How much the intensity varies (0.0-0.3)

    PointLight()
        : id(QUuid::createUuid())
        , position(0, 0)
        , color(255, 200, 100)  // Warm torch color
        , radius(200.0)
        , intensity(1.0)
        , falloff(1.5)          // Slightly softer than linear
        , enabled(true)
        , name()
        , flickering(false)
        , flickerAmount(0.1)
    {}

    PointLight(const QPointF& pos, qreal rad = 200.0, const QColor& col = QColor(255, 200, 100))
        : id(QUuid::createUuid())
        , position(pos)
        , color(col)
        , radius(rad)
        , intensity(1.0)
        , falloff(1.5)
        , enabled(true)
        , name()
        , flickering(false)
        , flickerAmount(0.1)
    {}

    // Preset light types
    static PointLight torch(const QPointF& pos) {
        PointLight light(pos, 150.0, QColor(255, 180, 80));
        light.flickering = true;
        light.flickerAmount = 0.15;
        light.name = "Torch";
        return light;
    }

    static PointLight lantern(const QPointF& pos) {
        PointLight light(pos, 200.0, QColor(255, 220, 150));
        light.flickering = true;
        light.flickerAmount = 0.05;  // Steadier than torch
        light.name = "Lantern";
        return light;
    }

    static PointLight campfire(const QPointF& pos) {
        PointLight light(pos, 300.0, QColor(255, 150, 50));
        light.flickering = true;
        light.flickerAmount = 0.2;
        light.name = "Campfire";
        return light;
    }

    static PointLight magical(const QPointF& pos, const QColor& col = QColor(100, 150, 255)) {
        PointLight light(pos, 180.0, col);
        light.flickering = false;  // Magical lights are steady
        light.falloff = 2.0;       // Sharper edge
        light.name = "Magical Light";
        return light;
    }

    static PointLight moonbeam(const QPointF& pos) {
        PointLight light(pos, 250.0, QColor(200, 220, 255));
        light.intensity = 0.7;
        light.falloff = 1.2;
        light.name = "Moonbeam";
        return light;
    }
};

// Light type presets for UI
enum class LightPreset {
    Torch,
    Lantern,
    Campfire,
    Magical,
    Moonbeam,
    Custom
};

#endif // POINTLIGHT_H
