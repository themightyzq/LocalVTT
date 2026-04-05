#ifndef ATMOSPHERESTATE_H
#define ATMOSPHERESTATE_H

#include <QColor>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QJsonObject>

// Atmosphere state for smooth transitions between environmental presets
struct AtmosphereState
{
    // Lighting
    int timeOfDay = 1;           // 0=Dawn, 1=Day, 2=Dusk, 3=Night
    qreal lightingIntensity = 1.0;
    QColor lightingTint = Qt::white;
    qreal ambientLevel = 0.2;
    qreal exposure = 1.0;

    // Weather (Phase 2)
    int weatherType = 0;         // 0=None, 1=Rain, 2=Snow, 3=Storm
    qreal weatherIntensity = 0.5;
    qreal windStrength = 0.3;
    qreal windDirection = 0.0;   // Angle in degrees

    // Fog/Mist (Phase 3)
    bool fogEnabled = false;
    qreal fogDensity = 0.3;
    qreal fogHeight = 0.3;       // 0.0-1.0, portion of screen from bottom
    QColor fogColor = QColor(180, 180, 200, 128);

    // Lightning (Phase 4)
    bool lightningEnabled = false;
    qreal lightningIntensity = 0.8;  // 0.0-1.0, max flash brightness
    qreal lightningFrequency = 0.5;  // 0.0-1.0, how often strikes occur

    // Audio
    QString ambientTrack;          // Filename relative to audio dir, or absolute path
    qreal ambientVolume = 0.7;     // 0.0 - 1.0
    QString musicURL;              // Spotify URI, Apple Music URL, or empty

    // Interpolate between two states
    static AtmosphereState lerp(const AtmosphereState& a,
                                 const AtmosphereState& b,
                                 qreal t)
    {
        AtmosphereState result;

        // Clamp t to [0, 1]
        t = qBound(0.0, t, 1.0);

        // Lighting interpolation
        // Time of day: snap at midpoint for discrete states
        result.timeOfDay = (t < 0.5) ? a.timeOfDay : b.timeOfDay;
        result.lightingIntensity = a.lightingIntensity + (b.lightingIntensity - a.lightingIntensity) * t;
        result.lightingTint = QColor(
            static_cast<int>(a.lightingTint.red() + (b.lightingTint.red() - a.lightingTint.red()) * t),
            static_cast<int>(a.lightingTint.green() + (b.lightingTint.green() - a.lightingTint.green()) * t),
            static_cast<int>(a.lightingTint.blue() + (b.lightingTint.blue() - a.lightingTint.blue()) * t)
        );
        result.ambientLevel = a.ambientLevel + (b.ambientLevel - a.ambientLevel) * t;
        result.exposure = a.exposure + (b.exposure - a.exposure) * t;

        // Weather interpolation
        result.weatherType = (t < 0.5) ? a.weatherType : b.weatherType;
        result.weatherIntensity = a.weatherIntensity + (b.weatherIntensity - a.weatherIntensity) * t;
        result.windStrength = a.windStrength + (b.windStrength - a.windStrength) * t;
        result.windDirection = a.windDirection + (b.windDirection - a.windDirection) * t;

        // Fog interpolation
        result.fogEnabled = (t < 0.5) ? a.fogEnabled : b.fogEnabled;
        result.fogDensity = a.fogDensity + (b.fogDensity - a.fogDensity) * t;
        result.fogHeight = a.fogHeight + (b.fogHeight - a.fogHeight) * t;
        result.fogColor = QColor(
            static_cast<int>(a.fogColor.red() + (b.fogColor.red() - a.fogColor.red()) * t),
            static_cast<int>(a.fogColor.green() + (b.fogColor.green() - a.fogColor.green()) * t),
            static_cast<int>(a.fogColor.blue() + (b.fogColor.blue() - a.fogColor.blue()) * t),
            static_cast<int>(a.fogColor.alpha() + (b.fogColor.alpha() - a.fogColor.alpha()) * t)
        );

        // Lightning interpolation (Phase 4)
        result.lightningEnabled = (t < 0.5) ? a.lightningEnabled : b.lightningEnabled;
        result.lightningIntensity = a.lightningIntensity + (b.lightningIntensity - a.lightningIntensity) * t;
        result.lightningFrequency = a.lightningFrequency + (b.lightningFrequency - a.lightningFrequency) * t;

        // Audio interpolation
        result.ambientTrack = (t < 0.5) ? a.ambientTrack : b.ambientTrack;
        result.ambientVolume = a.ambientVolume + (b.ambientVolume - a.ambientVolume) * t;
        result.musicURL = (t < 0.5) ? a.musicURL : b.musicURL;

        return result;
    }

    // Serialization for saving/loading
    QByteArray toByteArray() const
    {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << timeOfDay << lightingIntensity << lightingTint
               << ambientLevel << exposure
               << weatherType << weatherIntensity << windStrength << windDirection
               << fogEnabled << fogDensity << fogHeight << fogColor
               << lightningEnabled << lightningIntensity << lightningFrequency;
        return data;
    }

    static AtmosphereState fromByteArray(const QByteArray& data)
    {
        AtmosphereState state;
        QDataStream stream(data);
        stream >> state.timeOfDay >> state.lightingIntensity >> state.lightingTint
               >> state.ambientLevel >> state.exposure
               >> state.weatherType >> state.weatherIntensity
               >> state.windStrength >> state.windDirection
               >> state.fogEnabled >> state.fogDensity
               >> state.fogHeight >> state.fogColor;
        // Lightning fields may not exist in older saves - check stream status
        if (!stream.atEnd()) {
            stream >> state.lightningEnabled >> state.lightningIntensity
                   >> state.lightningFrequency;
        }
        return state;
    }

    // JSON serialization for custom presets
    QJsonObject toJson() const
    {
        QJsonObject obj;
        // Lighting
        obj["timeOfDay"] = timeOfDay;
        obj["lightingIntensity"] = lightingIntensity;
        obj["lightingTint"] = QJsonObject{
            {"r", lightingTint.red()},
            {"g", lightingTint.green()},
            {"b", lightingTint.blue()}
        };
        obj["ambientLevel"] = ambientLevel;
        obj["exposure"] = exposure;
        // Weather
        obj["weatherType"] = weatherType;
        obj["weatherIntensity"] = weatherIntensity;
        obj["windStrength"] = windStrength;
        obj["windDirection"] = windDirection;
        // Fog
        obj["fogEnabled"] = fogEnabled;
        obj["fogDensity"] = fogDensity;
        obj["fogHeight"] = fogHeight;
        obj["fogColor"] = QJsonObject{
            {"r", fogColor.red()},
            {"g", fogColor.green()},
            {"b", fogColor.blue()},
            {"a", fogColor.alpha()}
        };
        // Lightning
        obj["lightningEnabled"] = lightningEnabled;
        obj["lightningIntensity"] = lightningIntensity;
        obj["lightningFrequency"] = lightningFrequency;
        // Audio
        obj["ambientTrack"] = ambientTrack;
        obj["ambientVolume"] = ambientVolume;
        obj["musicURL"] = musicURL;
        return obj;
    }

    static AtmosphereState fromJson(const QJsonObject& obj)
    {
        AtmosphereState state;
        // Lighting
        state.timeOfDay = obj["timeOfDay"].toInt(1);
        state.lightingIntensity = obj["lightingIntensity"].toDouble(1.0);
        if (obj.contains("lightingTint")) {
            QJsonObject tint = obj["lightingTint"].toObject();
            state.lightingTint = QColor(tint["r"].toInt(255), tint["g"].toInt(255), tint["b"].toInt(255));
        }
        state.ambientLevel = obj["ambientLevel"].toDouble(0.2);
        state.exposure = obj["exposure"].toDouble(1.0);
        // Weather
        state.weatherType = obj["weatherType"].toInt(0);
        state.weatherIntensity = obj["weatherIntensity"].toDouble(0.5);
        state.windStrength = obj["windStrength"].toDouble(0.3);
        state.windDirection = obj["windDirection"].toDouble(0.0);
        // Fog
        state.fogEnabled = obj["fogEnabled"].toBool(false);
        state.fogDensity = obj["fogDensity"].toDouble(0.3);
        state.fogHeight = obj["fogHeight"].toDouble(0.3);
        if (obj.contains("fogColor")) {
            QJsonObject fc = obj["fogColor"].toObject();
            state.fogColor = QColor(fc["r"].toInt(180), fc["g"].toInt(180), fc["b"].toInt(200), fc["a"].toInt(128));
        }
        // Lightning
        state.lightningEnabled = obj["lightningEnabled"].toBool(false);
        state.lightningIntensity = obj["lightningIntensity"].toDouble(0.8);
        state.lightningFrequency = obj["lightningFrequency"].toDouble(0.5);
        // Audio
        state.ambientTrack = obj["ambientTrack"].toString();
        state.ambientVolume = obj["ambientVolume"].toDouble(0.7);
        state.musicURL = obj["musicURL"].toString();
        return state;
    }

    bool operator==(const AtmosphereState& other) const
    {
        return timeOfDay == other.timeOfDay &&
               qFuzzyCompare(lightingIntensity, other.lightingIntensity) &&
               lightingTint == other.lightingTint &&
               qFuzzyCompare(ambientLevel, other.ambientLevel) &&
               qFuzzyCompare(exposure, other.exposure) &&
               weatherType == other.weatherType &&
               qFuzzyCompare(weatherIntensity, other.weatherIntensity) &&
               fogEnabled == other.fogEnabled &&
               qFuzzyCompare(fogDensity, other.fogDensity) &&
               lightningEnabled == other.lightningEnabled &&
               qFuzzyCompare(lightningIntensity, other.lightningIntensity) &&
               qFuzzyCompare(lightningFrequency, other.lightningFrequency) &&
               ambientTrack == other.ambientTrack &&
               qFuzzyCompare(ambientVolume, other.ambientVolume) &&
               musicURL == other.musicURL;
    }

    bool operator!=(const AtmosphereState& other) const { return !(*this == other); }
};

#endif // ATMOSPHERESTATE_H
