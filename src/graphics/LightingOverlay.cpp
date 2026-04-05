#include "graphics/LightingOverlay.h"
#include "graphics/ZLayers.h"
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
    , m_useHDRLighting(true)
    , m_exposure(0.2)  // Minimal exposure for subtle lighting
    , m_brightness(0.0)  // Neutral brightness
    , m_contrast(0.0)    // Neutral contrast
{
    setZValue(ZLayer::LightingOverlay);
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

void LightingOverlay::renderBrightnessContrast(QPainter* painter)
{
    // Apply brightness/contrast overlay (DM only)
    // Brightness: -1.0 to 1.0, where < 0 darkens, > 0 brightens
    // Contrast: -1.0 to 1.0, where < 0 reduces contrast, > 0 increases contrast

    painter->save();

    // Brightness effect: overlay with white (brighten) or black (darken)
    if (m_brightness != 0.0) {
        if (m_brightness > 0.0) {
            // Brighten: overlay with semi-transparent white
            // Using CompositionMode_Plus adds light
            painter->setCompositionMode(QPainter::CompositionMode_Plus);
            int alpha = int(m_brightness * 100);  // Max 100 alpha for subtle effect
            painter->setBrush(QColor(255, 255, 255, alpha));
        } else {
            // Darken: multiply with gray (darker gray = more darkening)
            painter->setCompositionMode(QPainter::CompositionMode_Multiply);
            int grayValue = int(255 * (1.0 + m_brightness));  // 0 at brightness=-1, 255 at brightness=0
            painter->setBrush(QColor(grayValue, grayValue, grayValue, 255));
        }
        painter->setPen(Qt::NoPen);
        painter->drawRect(m_bounds);
    }

    // Contrast effect: overlay gray with SoftLight composition
    // Reduces contrast when applied with gray closer to 128
    // Note: QPainter doesn't have a perfect contrast mode, so we use an approximation
    if (m_contrast != 0.0) {
        if (m_contrast < 0.0) {
            // Reduce contrast: blend toward middle gray
            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
            int alpha = int(-m_contrast * 80);  // Subtle effect
            painter->setBrush(QColor(128, 128, 128, alpha));
        } else {
            // Increase contrast: use overlay mode with gray levels
            // This naturally increases contrast (darks get darker, lights get lighter)
            painter->setCompositionMode(QPainter::CompositionMode_Overlay);
            int alpha = int(m_contrast * 60);  // Subtle effect
            painter->setBrush(QColor(128, 128, 128, alpha));
        }
        painter->setPen(Qt::NoPen);
        painter->drawRect(m_bounds);
    }

    painter->restore();
}

void LightingOverlay::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)

    if (!m_enabled) {
        return;
    }

    // Save painter state
    painter->save();

    // First, render ambient lighting overlay (darkening effect)
    renderAmbientOverlay(painter);

    // Apply DM-only brightness/contrast adjustment
    // Use window detection pattern from CLAUDE.md
    bool isDMWindow = false;
    if (widget) {
        QWidget* window = widget->window();
        if (window && window->objectName() == "MainWindow") {
            isDMWindow = true;
        }
    }

    if (isDMWindow && (m_brightness != 0.0 || m_contrast != 0.0)) {
        renderBrightnessContrast(painter);
    }

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
