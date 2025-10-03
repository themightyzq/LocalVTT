#ifndef LIGHTINGOVERLAY_H
#define LIGHTINGOVERLAY_H

#include <QGraphicsItem>
#include <QColor>
#include <QRadialGradient>
#include <QList>
#include <QVector3D>

enum class TimeOfDay {
    Dawn,    // 0.8 intensity orange tint
    Day,     // 1.0 intensity no tint
    Dusk,    // 0.6 intensity orange-red tint
    Night    // 0.2 intensity blue tint
};

// Simple lighting overlay for time of day effects
class LightingOverlay : public QGraphicsItem
{
public:
    explicit LightingOverlay(QGraphicsItem* parent = nullptr);
    ~LightingOverlay();

    // Time of day controls
    void setTimeOfDay(TimeOfDay timeOfDay);
    TimeOfDay getTimeOfDay() const { return m_timeOfDay; }

    // Custom lighting controls
    void setLightingIntensity(qreal intensity); // 0.0 to 1.0
    qreal getLightingIntensity() const { return m_intensity; }

    void setLightingTint(const QColor& tint);
    QColor getLightingTint() const { return m_tint; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // VTT lighting data
    void setGlobalLight(bool enabled) { m_globalLight = enabled; }
    bool hasGlobalLight() const { return m_globalLight; }

    void setDarkness(qreal darkness); // 0.0 to 1.0
    qreal getDarkness() const { return m_darkness; }

    // Ambient lighting
    void setAmbientLightLevel(qreal level); // 0.0 to 1.0
    qreal getAmbientLightLevel() const { return m_ambientLightLevel; }

    // HDR lighting controls
    void setHDRLightingEnabled(bool enabled) { m_useHDRLighting = enabled; invalidateCache(); }
    bool isHDRLightingEnabled() const { return m_useHDRLighting; }

    void setExposure(qreal exposure) { m_exposure = qBound(0.1, exposure, 3.0); invalidateCache(); }
    qreal getExposure() const { return m_exposure; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void updateBounds();
    void applyTimeOfDaySettings();
    void renderAmbientOverlay(QPainter* painter);
    void invalidateCache();
    QColor calculateOptimizedOverlayColor(qreal finalIntensity) const;

    // HDR lighting functions (kept for tone mapping ambient light only)
    QColor applyToneMapping(const QVector3D& hdrColor) const;
    QVector3D sRGBToLinear(const QColor& color) const;
    QColor linearToSRGB(const QVector3D& linear) const;
    qreal reinhardToneMap(qreal hdrValue, qreal exposure = 1.0) const;

    TimeOfDay m_timeOfDay;
    qreal m_intensity;
    QColor m_tint;
    bool m_enabled;
    QRectF m_bounds;

    // VTT lighting data
    bool m_globalLight;
    qreal m_darkness;

    // Ambient lighting
    qreal m_ambientLightLevel;

    // Performance optimization
    mutable bool m_cacheValid;

    // Ambient overlay optimization
    mutable QColor m_cachedOverlayColor;

    // HDR lighting
    bool m_useHDRLighting;
    qreal m_exposure;  // HDR exposure control (0.1 to 3.0)
};

#endif // LIGHTINGOVERLAY_H