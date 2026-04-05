#ifndef ATMOSPHEREPRESET_H
#define ATMOSPHEREPRESET_H

#include "AtmosphereState.h"
#include <QString>
#include <QList>

// Pre-configured atmosphere presets for dramatic transitions
struct AtmospherePreset
{
    QString name;
    QString description;
    AtmosphereState state;

    AtmospherePreset() = default;
    AtmospherePreset(const QString& n, const QString& desc, const AtmosphereState& s)
        : name(n), description(desc), state(s) {}

    // Built-in presets
    static QList<AtmospherePreset> getBuiltInPresets()
    {
        QList<AtmospherePreset> presets;

        // === PHASE 1: Core Lighting Presets ===

        // Peaceful Day - Bright, neutral lighting
        {
            AtmosphereState state;
            state.timeOfDay = 1;  // Day
            state.lightingIntensity = 1.0;
            state.lightingTint = Qt::white;
            state.ambientLevel = 0.3;
            state.exposure = 1.0;
            state.weatherType = 0;  // None
            state.fogEnabled = false;
            state.ambientTrack = "nature/forest-birds.ogg";
            state.ambientVolume = 0.5;
            presets.append(AtmospherePreset(
                "Peaceful Day",
                "Bright daylight with neutral tones",
                state
            ));
        }

        // Golden Dawn - Warm morning light with subtle mist
        {
            AtmosphereState state;
            state.timeOfDay = 0;  // Dawn
            state.lightingIntensity = 0.85;
            state.lightingTint = QColor(255, 220, 180);  // Warm orange
            state.ambientLevel = 0.25;
            state.exposure = 0.95;
            state.weatherType = 0;
            state.fogEnabled = true;
            state.fogDensity = 0.15;
            state.fogHeight = 0.2;
            state.fogColor = QColor(255, 240, 220, 60);  // Warm mist
            state.ambientTrack = "nature/wind-gentle.ogg";
            state.ambientVolume = 0.4;
            presets.append(AtmospherePreset(
                "Golden Dawn",
                "Warm sunrise with light morning mist",
                state
            ));
        }

        // Warm Dusk - Orange-red sunset
        {
            AtmosphereState state;
            state.timeOfDay = 2;  // Dusk
            state.lightingIntensity = 0.7;
            state.lightingTint = QColor(255, 150, 100);  // Orange-red
            state.ambientLevel = 0.2;
            state.exposure = 0.85;
            state.weatherType = 0;
            state.fogEnabled = false;
            state.ambientTrack = "nature/wind-gentle.ogg";
            state.ambientVolume = 0.3;
            presets.append(AtmospherePreset(
                "Warm Dusk",
                "Dramatic orange-red sunset",
                state
            ));
        }

        // Moonlit Night - Cool blue nighttime
        {
            AtmosphereState state;
            state.timeOfDay = 3;  // Night
            state.lightingIntensity = 0.4;
            state.lightingTint = QColor(150, 180, 220);  // Cool blue
            state.ambientLevel = 0.1;
            state.exposure = 0.6;
            state.weatherType = 0;
            state.fogEnabled = false;
            state.ambientTrack = "nature/wind-gentle.ogg";
            state.ambientVolume = 0.5;
            presets.append(AtmospherePreset(
                "Moonlit Night",
                "Cool blue moonlight",
                state
            ));
        }

        // === PHASE 2: Weather Presets (placeholders for future) ===

        // Stormy Night - Dark with rain and lightning
        {
            AtmosphereState state;
            state.timeOfDay = 3;  // Night
            state.lightingIntensity = 0.25;
            state.lightingTint = QColor(100, 110, 130);  // Dark blue-gray
            state.ambientLevel = 0.08;
            state.exposure = 0.5;
            state.weatherType = 3;  // Storm
            state.weatherIntensity = 0.8;
            state.windStrength = 0.7;
            state.windDirection = 45.0;
            state.fogEnabled = true;
            state.fogDensity = 0.3;
            state.fogHeight = 0.4;
            state.fogColor = QColor(80, 90, 110, 100);
            state.lightningEnabled = true;
            state.lightningIntensity = 0.85;
            state.lightningFrequency = 0.6;
            state.ambientTrack = "nature/rain-heavy.ogg";
            state.ambientVolume = 0.8;
            presets.append(AtmospherePreset(
                "Stormy Night",
                "Dark and ominous with heavy rain and lightning",
                state
            ));
        }

        // Light Rain - Gentle daytime rain with misty atmosphere
        {
            AtmosphereState state;
            state.timeOfDay = 1;  // Day
            state.lightingIntensity = 0.6;
            state.lightingTint = QColor(180, 190, 200);  // Overcast gray
            state.ambientLevel = 0.25;
            state.exposure = 0.8;
            state.weatherType = 1;  // Rain
            state.weatherIntensity = 0.4;
            state.windStrength = 0.2;
            state.windDirection = 0.0;
            state.fogEnabled = true;
            state.fogDensity = 0.25;  // More visible mist
            state.fogHeight = 0.35;   // Higher coverage
            state.fogColor = QColor(180, 190, 200, 80);  // Match overcast gray
            state.ambientTrack = "nature/rain-light.ogg";
            state.ambientVolume = 0.6;
            presets.append(AtmospherePreset(
                "Light Rain",
                "Gentle overcast rain with mist",
                state
            ));
        }

        // Winter Snow - Cold and snowy with wintry haze
        {
            AtmosphereState state;
            state.timeOfDay = 1;  // Day
            state.lightingIntensity = 0.9;
            state.lightingTint = QColor(230, 240, 255);  // Cool white
            state.ambientLevel = 0.35;
            state.exposure = 1.1;  // Slightly bright (snow reflection)
            state.weatherType = 2;  // Snow
            state.weatherIntensity = 0.5;
            state.windStrength = 0.15;
            state.windDirection = 30.0;
            state.fogEnabled = true;
            state.fogDensity = 0.3;   // More visible winter haze
            state.fogHeight = 0.4;    // Higher coverage
            state.fogColor = QColor(230, 240, 255, 100);  // Bright cold mist
            state.ambientTrack = "nature/wind-gentle.ogg";
            state.ambientVolume = 0.4;
            presets.append(AtmospherePreset(
                "Winter Snow",
                "Cold winter day with snow and haze",
                state
            ));
        }

        // === PHASE 3: Atmospheric Presets ===

        // Dungeon Depths - Dark and eerie with ethereal mist
        {
            AtmosphereState state;
            state.timeOfDay = 3;  // Night (no natural light)
            state.lightingIntensity = 0.2;
            state.lightingTint = QColor(140, 130, 120);  // Warm torchlight hint
            state.ambientLevel = 0.05;
            state.exposure = 0.4;
            state.weatherType = 0;
            state.fogEnabled = true;
            state.fogDensity = 0.4;
            state.fogHeight = 0.6;
            state.fogColor = QColor(180, 200, 230, 120);  // Blueish-white ethereal mist
            state.ambientTrack = "dungeon/cave-dripping.ogg";
            state.ambientVolume = 0.6;
            presets.append(AtmospherePreset(
                "Dungeon Depths",
                "Dark underground with ethereal mist",
                state
            ));
        }

        // Mystical Fog - Ethereal and mysterious
        {
            AtmosphereState state;
            state.timeOfDay = 2;  // Dusk
            state.lightingIntensity = 0.5;
            state.lightingTint = QColor(180, 160, 200);  // Purple tint
            state.ambientLevel = 0.15;
            state.exposure = 0.7;
            state.weatherType = 0;
            state.fogEnabled = true;
            state.fogDensity = 0.5;
            state.fogHeight = 0.5;
            state.fogColor = QColor(160, 140, 180, 120);  // Purple mist
            state.ambientTrack = "dungeon/underground-rumble.ogg";
            state.ambientVolume = 0.3;
            presets.append(AtmospherePreset(
                "Mystical Fog",
                "Ethereal purple mist for magical locations",
                state
            ));
        }

        // Volcanic - Hot and dangerous
        {
            AtmosphereState state;
            state.timeOfDay = 3;  // Night (dark sky)
            state.lightingIntensity = 0.35;
            state.lightingTint = QColor(255, 120, 80);  // Hot orange-red
            state.ambientLevel = 0.1;
            state.exposure = 0.65;
            state.weatherType = 0;
            state.fogEnabled = true;
            state.fogDensity = 0.35;
            state.fogHeight = 0.4;
            state.fogColor = QColor(80, 40, 30, 100);  // Ash/smoke
            state.ambientTrack = "dungeon/underground-rumble.ogg";
            state.ambientVolume = 0.7;
            presets.append(AtmospherePreset(
                "Volcanic",
                "Hot volcanic environment with ash",
                state
            ));
        }

        return presets;
    }

    // Get a preset by name
    static AtmospherePreset getPresetByName(const QString& name)
    {
        for (const auto& preset : getBuiltInPresets()) {
            if (preset.name == name) {
                return preset;
            }
        }
        // Return default (Peaceful Day) if not found
        return getBuiltInPresets().first();
    }

    // Get just the Phase 1 presets (lighting only, no weather/fog effects)
    static QList<AtmospherePreset> getPhase1Presets()
    {
        QList<AtmospherePreset> phase1;
        auto all = getBuiltInPresets();
        // First 4 are Phase 1: Peaceful Day, Golden Dawn, Warm Dusk, Moonlit Night
        for (int i = 0; i < 4 && i < all.size(); ++i) {
            phase1.append(all[i]);
        }
        return phase1;
    }

    // Get Phase 2 presets (includes weather effects)
    static QList<AtmospherePreset> getPhase2Presets()
    {
        QList<AtmospherePreset> phase2;
        auto all = getBuiltInPresets();
        // Weather presets: Stormy Night, Light Rain, Winter Snow (indices 4-6)
        for (int i = 4; i < 7 && i < all.size(); ++i) {
            phase2.append(all[i]);
        }
        return phase2;
    }

    // Get all presets with weather effects (weatherType > 0)
    static QList<AtmospherePreset> getWeatherPresets()
    {
        QList<AtmospherePreset> weather;
        for (const auto& preset : getBuiltInPresets()) {
            if (preset.state.weatherType > 0) {
                weather.append(preset);
            }
        }
        return weather;
    }

    // Get Phase 3 presets (atmospheric fog effects)
    static QList<AtmospherePreset> getPhase3Presets()
    {
        QList<AtmospherePreset> phase3;
        auto all = getBuiltInPresets();
        // Fog presets: Dungeon Depths, Mystical Fog, Volcanic (indices 7-9)
        for (int i = 7; i < 10 && i < all.size(); ++i) {
            phase3.append(all[i]);
        }
        return phase3;
    }

    // Get all presets with fog effects enabled
    static QList<AtmospherePreset> getFogPresets()
    {
        QList<AtmospherePreset> fogPresets;
        for (const auto& preset : getBuiltInPresets()) {
            if (preset.state.fogEnabled) {
                fogPresets.append(preset);
            }
        }
        return fogPresets;
    }
};

#endif // ATMOSPHEREPRESET_H
