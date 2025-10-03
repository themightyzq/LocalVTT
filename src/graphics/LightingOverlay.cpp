#include "graphics/LightingOverlay.h"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtMath>
#include <QVector3D>

LightingOverlay::LightingOverlay(QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_timeOfDay(TimeOfDay::Day)
    , m_intensity(1.0)
    , m_tint(Qt::white)
    , m_enabled(true)  // Enable lighting by default for proper menu state
    , m_bounds(0, 0, 1920, 1080)
    , m_globalLight(true)
    , m_darkness(0.0)
    , m_ambientLightLevel(0.2)
    , m_cacheValid(false)
    , m_exposure(0.2)  // Minimal exposure for subtle lighting
    , m_useHDRLighting(true)
{
    setZValue(600); // Above weather effects but below UI
    setVisible(true);  // Make visible since enabled by default
    applyTimeOfDaySettings();
}

LightingOverlay::~LightingOverlay()
{
    // Note: Qt's parent-child system handles scene item deletion
}

void LightingOverlay::setTimeOfDay(TimeOfDay timeOfDay)
{
    m_timeOfDay = timeOfDay;
    applyTimeOfDaySettings();
    invalidateCache();
    update();
}

void LightingOverlay::setLightingIntensity(qreal intensity)
{
    m_intensity = qBound(0.0, intensity, 1.0);
    invalidateCache();
    update();
}

void LightingOverlay::setLightingTint(const QColor& tint)
{
    m_tint = tint;
    invalidateCache();
    update();
}

void LightingOverlay::setEnabled(bool enabled)
{
    m_enabled = enabled;
    setVisible(enabled);

    if (enabled) {
        updateBounds();
        invalidateCache();
    }
}

void LightingOverlay::setDarkness(qreal darkness)
{
    m_darkness = qBound(0.0, darkness, 1.0);
    invalidateCache();
    update();
}

void LightingOverlay::setAmbientLightLevel(qreal level)
{
    m_ambientLightLevel = qBound(0.0, level, 1.0);
    invalidateCache();
    update();
}

// Point light methods removed - keeping only basic ambient lighting

QColor LightingOverlay::calculateOptimizedOverlayColor(qreal finalIntensity) const
{
    QColor overlayColor = m_tint;

    if (finalIntensity < 1.0) {
        // Darken the scene by blending with black
        qreal darkenAmount = 1.0 - finalIntensity;
        overlayColor = QColor(
            int(m_tint.red() * finalIntensity),
            int(m_tint.green() * finalIntensity),
            int(m_tint.blue() * finalIntensity),
            int(darkenAmount * 200) // Stronger darkening for better contrast
        );
    } else {
        // Just apply tint
        overlayColor.setAlpha(80); // Lighter tint
    }

    return overlayColor;
}

QRectF LightingOverlay::boundingRect() const
{
    return m_bounds;
}

void LightingOverlay::updateBounds()
{
    if (scene() && !scene()->views().isEmpty()) {
        QGraphicsView* view = scene()->views().first();
        QRectF sceneRect = scene()->sceneRect();
        QRectF viewportRect = view->mapToScene(view->viewport()->rect()).boundingRect();

        m_bounds = sceneRect.united(viewportRect);

        if (m_bounds.width() < 1920) {
            m_bounds.setWidth(1920);
        }
        if (m_bounds.height() < 1080) {
            m_bounds.setHeight(1080);
        }

        invalidateCache();
        prepareGeometryChange();
    }
}

void LightingOverlay::applyTimeOfDaySettings()
{
    switch (m_timeOfDay) {
        case TimeOfDay::Dawn:
            m_intensity = 0.8;
            m_tint = QColor(255, 200, 150); // Warm orange
            break;
        case TimeOfDay::Day:
            m_intensity = 1.0;
            m_tint = Qt::white; // No tint
            break;
        case TimeOfDay::Dusk:
            m_intensity = 0.6;
            m_tint = QColor(255, 150, 100); // Orange-red
            break;
        case TimeOfDay::Night:
            m_intensity = 0.2;
            m_tint = QColor(150, 150, 255); // Cool blue
            break;
    }
}

void LightingOverlay::invalidateCache()
{
    m_cacheValid = false;
}

// Point light gradient creation removed - only ambient lighting now

void LightingOverlay::renderAmbientOverlay(QPainter* painter)
{
    // Calculate effective lighting intensity
    qreal effectiveIntensity = m_intensity;
    if (!m_globalLight) {
        effectiveIntensity *= (1.0 - m_darkness);
    }
    qreal finalIntensity = qMax(effectiveIntensity, m_ambientLightLevel);

    // Only apply overlay if there's a significant lighting effect
    if (finalIntensity >= 0.95 && m_tint == Qt::white) {
        return; // No visible effect, skip rendering
    }

    // Calculate overlay color
    QColor overlayColor = calculateOptimizedOverlayColor(finalIntensity);

    // Set composition mode based on intensity
    if (finalIntensity < 1.0) {
        painter->setCompositionMode(QPainter::CompositionMode_Multiply);
    } else {
        painter->setCompositionMode(QPainter::CompositionMode_Overlay);
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(overlayColor));
    painter->drawRect(m_bounds);
}

void LightingOverlay::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!m_enabled) {
        return;
    }

    // Save painter state
    painter->save();

    // First, render ambient lighting overlay (darkening effect)
    renderAmbientOverlay(painter);

    // Point lights removed - only ambient lighting now

    // Restore painter state
    painter->restore();
}

// HDR lighting functions for proper tone mapping and light accumulation

QVector3D LightingOverlay::sRGBToLinear(const QColor& color) const
{
    // Convert sRGB color to linear RGB space for proper light calculations
    auto gammaCorrect = [](qreal channel) -> qreal {
        qreal normalized = channel / 255.0;
        if (normalized <= 0.04045) {
            return normalized / 12.92;
        } else {
            return qPow((normalized + 0.055) / 1.055, 2.4);
        }
    };

    return QVector3D(
        gammaCorrect(color.red()),
        gammaCorrect(color.green()),
        gammaCorrect(color.blue())
    );
}

QColor LightingOverlay::linearToSRGB(const QVector3D& linear) const
{
    // Convert linear RGB back to sRGB for display
    auto inverseGammaCorrect = [](qreal channel) -> int {
        qreal corrected;
        if (channel <= 0.0031308) {
            corrected = channel * 12.92;
        } else {
            corrected = 1.055 * qPow(channel, 1.0 / 2.4) - 0.055;
        }
        return qBound(0, int(corrected * 255.0), 255);
    };

    return QColor(
        inverseGammaCorrect(linear.x()),
        inverseGammaCorrect(linear.y()),
        inverseGammaCorrect(linear.z())
    );
}

qreal LightingOverlay::reinhardToneMap(qreal hdrValue, qreal exposure) const
{
    // Reinhard tone mapping to prevent overexposure
    // Formula: L_out = L_in / (1 + L_in) where L_in = exposure * L_hdr
    qreal exposedValue = hdrValue * exposure;
    return exposedValue / (1.0 + exposedValue);
}

QColor LightingOverlay::applyToneMapping(const QVector3D& hdrColor) const
{
    // Apply Reinhard tone mapping to each channel
    QVector3D toneMapped(
        reinhardToneMap(hdrColor.x(), m_exposure),
        reinhardToneMap(hdrColor.y(), m_exposure),
        reinhardToneMap(hdrColor.z(), m_exposure)
    );

    // Convert back to sRGB
    return linearToSRGB(toneMapped);
}

// HDR light accumulation buffer removed - not needed without point lights

// HDR point light rendering removed - not needed without point lights
