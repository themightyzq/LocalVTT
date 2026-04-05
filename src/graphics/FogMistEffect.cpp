#include "FogMistEffect.h"
#include "graphics/ZLayers.h"
#include <QPainter>
#include <QDateTime>
#include <QRandomGenerator>
#include <QtMath>
#include <QLinearGradient>
#include "utils/DebugConsole.h"

FogMistEffect::FogMistEffect(QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_density(0.0)
    , m_height(0.3)
    , m_color(QColor(200, 200, 200, 100))
    , m_enabled(false)
    , m_animationSpeed(0.5)
    , m_sceneScale(1.0)
    , m_animationTimer(new QTimer(this))
    , m_animationOffset(0.0)
    , m_lastAnimationTime(0)
    , m_hasTexture(false)
    , m_textureScale(1.0)
    , m_textureTwist(0.3)
    , m_transitionTimer(new QTimer(this))
    , m_startDensity(0.0)
    , m_targetDensity(0.0)
    , m_startHeight(0.3)
    , m_targetHeight(0.3)
    , m_transitionStartTime(0)
    , m_transitionDuration(1000)
    , m_isTransitioning(false)
{
    setZValue(ZLayer::FogMist);

    // Generate noise texture for organic fog
    generateNoiseTexture();

    // Setup animation timer
    connect(m_animationTimer, &QTimer::timeout, this, &FogMistEffect::onAnimationTick);

    // Setup transition timer
    connect(m_transitionTimer, &QTimer::timeout, this, &FogMistEffect::onTransitionTick);

    // Initialize last animation time
    m_lastAnimationTime = QDateTime::currentMSecsSinceEpoch();
}

FogMistEffect::~FogMistEffect()
{
    if (m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
    if (m_transitionTimer->isActive()) {
        m_transitionTimer->stop();
    }
}

QRectF FogMistEffect::boundingRect() const
{
    if (m_sceneBounds.isValid()) {
        return m_sceneBounds;
    }
    return QRectF(-5000, -5000, 10000, 10000);
}

void FogMistEffect::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/,
                          QWidget* /*widget*/)
{
    if (!m_enabled || m_density <= 0.0 || m_height <= 0.0) {
        return;
    }

    if (!m_sceneBounds.isValid()) {
        return;
    }

    // Fast path: use pre-rendered frame from advanceAnimation()
    if (m_frameValid && !m_renderedFrame.isNull()) {
        painter->drawPixmap(m_sceneBounds.topLeft(), m_renderedFrame);
        return;
    }

    // Fallback: render directly (first frame before animation starts)
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    QRectF fogRect = m_sceneBounds;

    // Use scene scale for size calculations
    qreal scale = m_sceneScale;
    qreal baseCloudUnit = 100.0 * scale;

    // Calculate base alpha
    int baseAlpha = static_cast<int>(qBound(0, static_cast<int>(180 * m_density * m_height), 255));

    painter->setPen(Qt::NoPen);

    // ========================================================================
    // LAYER 1: Light base tint
    // ========================================================================
    QColor baseFogColor = m_color;
    baseFogColor.setAlpha(baseAlpha / 4);
    painter->fillRect(fogRect, baseFogColor);

    // ========================================================================
    // LAYER 2: Large billowing clouds - SIMPLIFIED for performance
    // Only 4-6 large clouds with simpler movement
    // ========================================================================
    const int bigCloudCount = 4 + static_cast<int>(m_density * 2);

    for (int i = 0; i < bigCloudCount; ++i) {
        // Simple sinusoidal movement
        qreal phase = m_animationOffset * 0.005 + i * 1.618;

        qreal driftX = qSin(phase) * fogRect.width() * 0.25;
        qreal driftY = qCos(phase * 0.7) * fogRect.height() * 0.25;

        qreal cloudX = fogRect.center().x() + driftX;
        qreal cloudY = fogRect.center().y() + driftY;

        qreal cloudRadius = baseCloudUnit * (12.0 + 4.0 * m_density);

        // Simpler 3-stop gradient
        QRadialGradient cloudGrad(cloudX, cloudY, cloudRadius);
        int coreAlpha = qBound(0, static_cast<int>(baseAlpha * 0.7), 255);

        cloudGrad.setColorAt(0.0, QColor(m_color.red(), m_color.green(), m_color.blue(), coreAlpha));
        cloudGrad.setColorAt(0.5, QColor(m_color.red(), m_color.green(), m_color.blue(), coreAlpha / 3));
        cloudGrad.setColorAt(1.0, Qt::transparent);

        painter->setBrush(cloudGrad);
        painter->drawEllipse(QPointF(cloudX, cloudY), cloudRadius, cloudRadius * 0.7);
    }

    // ========================================================================
    // LAYER 3: Medium puffs - SIMPLIFIED to just 4-8 puffs total
    // ========================================================================
    const int puffCount = 4 + static_cast<int>(m_density * 4);

    for (int i = 0; i < puffCount; ++i) {
        // Distribute around the scene
        qreal angle = i * 2.399 + m_animationOffset * 0.003;
        qreal radius = fogRect.width() * 0.3;

        qreal puffX = fogRect.center().x() + qCos(angle) * radius;
        qreal puffY = fogRect.center().y() + qSin(angle) * radius;

        qreal puffRadius = baseCloudUnit * (5.0 + 2.0 * m_density);

        QRadialGradient puffGrad(puffX, puffY, puffRadius);
        int puffAlpha = qBound(0, static_cast<int>(baseAlpha * 0.5), 255);

        puffGrad.setColorAt(0.0, QColor(m_color.red(), m_color.green(), m_color.blue(), puffAlpha));
        puffGrad.setColorAt(0.6, QColor(m_color.red(), m_color.green(), m_color.blue(), puffAlpha / 4));
        puffGrad.setColorAt(1.0, Qt::transparent);

        painter->setBrush(puffGrad);
        painter->drawEllipse(QPointF(puffX, puffY), puffRadius, puffRadius * 0.8);
    }

    // ========================================================================
    // LAYER 4: Small wisps - only 4-6 for detail
    // ========================================================================
    const int wispCount = 4 + static_cast<int>(m_density * 2);

    for (int i = 0; i < wispCount; ++i) {
        qreal phase = m_animationOffset * 0.01 + i * 1.3;

        qreal wispX = fogRect.center().x() + qSin(phase + i) * fogRect.width() * 0.35;
        qreal wispY = fogRect.center().y() + qCos(phase * 0.8 + i) * fogRect.height() * 0.35;

        qreal wispRadius = baseCloudUnit * (3.0 + m_density);

        QRadialGradient wispGrad(wispX, wispY, wispRadius);
        int wispAlpha = qBound(0, static_cast<int>(baseAlpha * 0.4), 255);

        wispGrad.setColorAt(0.0, QColor(m_color.red(), m_color.green(), m_color.blue(), wispAlpha));
        wispGrad.setColorAt(1.0, Qt::transparent);

        painter->setBrush(wispGrad);
        painter->drawEllipse(QPointF(wispX, wispY), wispRadius, wispRadius * 0.6);
    }

    // ========================================================================
    // LAYER 5: Texture layer (if loaded) - scrolling/twisting seamless tile
    // ========================================================================
    if (m_hasTexture) {
        paintTextureLayer(painter, fogRect, baseAlpha);
    }

    painter->restore();
}

void FogMistEffect::setDensity(qreal density)
{
    m_density = qBound(0.0, density, 1.0);
    if (m_enabled) update();
}

void FogMistEffect::setHeight(qreal height)
{
    m_height = qBound(0.0, height, 1.0);
    if (m_enabled) update();
}

void FogMistEffect::setColor(const QColor& color)
{
    m_color = color;
    if (m_enabled) update();
}

void FogMistEffect::setSceneBounds(const QRectF& bounds)
{
    if (m_sceneBounds != bounds) {
        prepareGeometryChange();
        m_sceneBounds = bounds;

        // Calculate scale factor based on scene size
        qreal avgDimension = (bounds.width() + bounds.height()) / 2.0;
        m_sceneScale = qMax(1.0, avgDimension / 1000.0);

        // Invalidate cached scaled textures since fog rect affects tile sizes
        for (int i = 0; i < NUM_TEXTURE_LAYERS; ++i) {
            m_cachedTileSizes[i] = QSize();  // Force rescale on next paint
        }

        DebugConsole::info(
            QString("FogMistEffect: Scene bounds set to %1x%2 scale: %3")
                .arg(bounds.width(), 0, 'f', 0)
                .arg(bounds.height(), 0, 'f', 0)
                .arg(m_sceneScale, 0, 'f', 2),
            "Atmosphere");
    }
}

void FogMistEffect::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;

        if (enabled && m_density > 0.0) {
            m_lastAnimationTime = QDateTime::currentMSecsSinceEpoch();
            // Timer no longer auto-started — SceneAnimationDriver calls advanceAnimation()
        } else {
            m_animationTimer->stop();
        }

        update();
    }
}

void FogMistEffect::setAnimationSpeed(qreal speed)
{
    m_animationSpeed = qBound(0.0, speed, 2.0);
}

void FogMistEffect::transitionTo(qreal density, qreal height, const QColor& color, int durationMs)
{
    if (qFuzzyCompare(density, m_density) &&
        qFuzzyCompare(height, m_height) &&
        color == m_color) {
        return;
    }

    m_startDensity = m_density;
    m_targetDensity = density;
    m_startHeight = m_height;
    m_targetHeight = height;
    m_startColor = m_color;
    m_targetColor = color;
    m_transitionDuration = durationMs;
    m_transitionStartTime = QDateTime::currentMSecsSinceEpoch();
    m_isTransitioning = true;

    // Enable if transitioning to non-zero density
    if (!m_enabled && density > 0.0) {
        setEnabled(true);
    }

    m_transitionTimer->start(TRANSITION_INTERVAL_MS);
}

void FogMistEffect::renderFrame()
{
    if (!m_enabled || m_density <= 0.0 || !m_sceneBounds.isValid()) {
        m_frameValid = false;
        return;
    }

    QSize targetSize = m_sceneBounds.size().toSize();
    if (targetSize.isEmpty()) {
        m_frameValid = false;
        return;
    }

    if (m_renderedFrame.size() != targetSize) {
        m_renderedFrame = QPixmap(targetSize);
    }
    m_renderedFrame.fill(Qt::transparent);

    QPainter painter(&m_renderedFrame);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.translate(-m_sceneBounds.topLeft());

    // Reuse existing paint logic (call the original drawing code)
    painter.setPen(Qt::NoPen);
    QRectF fogRect = m_sceneBounds;
    qreal scale = m_sceneScale;
    qreal baseCloudUnit = 100.0 * scale;
    int baseAlpha = static_cast<int>(qBound(0, static_cast<int>(180 * m_density * m_height), 255));

    // Layer 1: base tint
    QColor baseFogColor = m_color;
    baseFogColor.setAlpha(baseAlpha / 4);
    painter.fillRect(fogRect, baseFogColor);

    // Layer 2: clouds (simplified — just draw the gradients)
    const int bigCloudCount = 4 + static_cast<int>(m_density * 2);
    for (int i = 0; i < bigCloudCount; ++i) {
        qreal phase = m_animationOffset * 0.005 + i * 1.618;
        qreal driftX = qSin(phase) * fogRect.width() * 0.25;
        qreal driftY = qCos(phase * 0.7) * fogRect.height() * 0.25;
        qreal cloudX = fogRect.center().x() + driftX;
        qreal cloudY = fogRect.center().y() + driftY;
        qreal cloudRadius = baseCloudUnit * (12.0 + 4.0 * m_density);

        QRadialGradient gradient(QPointF(cloudX, cloudY), cloudRadius);
        QColor cloudColor = m_color;
        cloudColor.setAlpha(baseAlpha / 2);
        gradient.setColorAt(0, cloudColor);
        cloudColor.setAlpha(0);
        gradient.setColorAt(1.0, cloudColor);
        painter.setBrush(QBrush(gradient));
        painter.drawEllipse(QPointF(cloudX, cloudY), cloudRadius, cloudRadius);
    }

    // Layer 3: texture (if available and density warrants it)
    if (m_hasTexture && m_density >= 0.3) {
        paintTextureLayer(&painter, fogRect, baseAlpha);
    }

    painter.end();
    m_frameValid = true;
}

void FogMistEffect::advanceAnimation(qreal dt)
{
    if (!m_enabled) {
        return;
    }

    // Update animation offset using driver-provided dt
    m_animationOffset += dt * m_animationSpeed * 50.0;

    // Wrap offset to prevent overflow
    if (m_animationOffset > 10000.0) {
        m_animationOffset -= 10000.0;
    }

    // Pre-render to offscreen pixmap (paint() will just blit this)
    renderFrame();
}

void FogMistEffect::onAnimationTick()
{
    if (!m_enabled) {
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qreal deltaTime = (currentTime - m_lastAnimationTime) / 1000.0;
    m_lastAnimationTime = currentTime;

    // Update animation offset
    m_animationOffset += deltaTime * m_animationSpeed * 50.0;

    // Wrap offset to prevent overflow
    if (m_animationOffset > 10000.0) {
        m_animationOffset -= 10000.0;
    }

    update();
}

void FogMistEffect::onTransitionTick()
{
    if (!m_isTransitioning) {
        m_transitionTimer->stop();
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_transitionStartTime;
    qreal progress = qBound(0.0, static_cast<qreal>(elapsed) / m_transitionDuration, 1.0);

    // Smooth easing (ease-in-out)
    qreal easedProgress = 0.5 - 0.5 * qCos(progress * M_PI);

    // Interpolate values
    m_density = m_startDensity + (m_targetDensity - m_startDensity) * easedProgress;
    m_height = m_startHeight + (m_targetHeight - m_startHeight) * easedProgress;

    // Interpolate color
    int r = m_startColor.red() + static_cast<int>((m_targetColor.red() - m_startColor.red()) * easedProgress);
    int g = m_startColor.green() + static_cast<int>((m_targetColor.green() - m_startColor.green()) * easedProgress);
    int b = m_startColor.blue() + static_cast<int>((m_targetColor.blue() - m_startColor.blue()) * easedProgress);
    int a = m_startColor.alpha() + static_cast<int>((m_targetColor.alpha() - m_startColor.alpha()) * easedProgress);
    m_color = QColor(r, g, b, a);

    if (progress >= 1.0) {
        m_isTransitioning = false;
        m_transitionTimer->stop();
        m_density = m_targetDensity;
        m_height = m_targetHeight;
        m_color = m_targetColor;

        // Disable if transitioned to zero density
        if (m_targetDensity <= 0.0) {
            setEnabled(false);
        }

        emit transitionCompleted();
        DebugConsole::info(
            QString("FogMistEffect: Transition complete - density: %1 height: %2")
                .arg(m_density, 0, 'f', 2)
                .arg(m_height, 0, 'f', 2),
            "Atmosphere");
    }

    update();
}

void FogMistEffect::generateNoiseTexture()
{
    // Generate a smooth Perlin-like noise texture for organic fog movement
    m_noiseTexture = QImage(NOISE_SIZE, NOISE_SIZE, QImage::Format_ARGB32);
    m_noiseTexture.fill(Qt::transparent);

    // Generate base noise with larger scale for smoother appearance
    for (int y = 0; y < NOISE_SIZE; ++y) {
        for (int x = 0; x < NOISE_SIZE; ++x) {
            // Use multiple octaves for more natural noise
            qreal noise = 0.0;
            qreal amplitude = 1.0;
            qreal frequency = 0.5;  // Start with lower frequency for larger features

            for (int octave = 0; octave < 3; ++octave) {  // Fewer octaves = smoother
                // Simple value noise (pseudo-random based on position)
                int sampleX = static_cast<int>(x * frequency) % NOISE_SIZE;
                int sampleY = static_cast<int>(y * frequency) % NOISE_SIZE;

                // Use a seeded random value based on position
                uint seed = static_cast<uint>(sampleX * 374761393 + sampleY * 668265263);
                qreal value = (seed % 1000) / 1000.0;

                noise += value * amplitude;
                amplitude *= 0.5;
                frequency *= 2.0;
            }

            // Normalize and apply to alpha channel
            int alpha = static_cast<int>(qBound(0.0, noise * 0.6, 1.0) * 255);
            m_noiseTexture.setPixelColor(x, y, QColor(255, 255, 255, alpha));
        }
    }

    // Apply multiple passes of Gaussian blur for very smooth noise
    // This eliminates blockiness and creates smooth gradients
    for (int pass = 0; pass < 3; ++pass) {
        QImage blurred(NOISE_SIZE, NOISE_SIZE, QImage::Format_ARGB32);
        blurred.fill(Qt::transparent);

        // 5x5 Gaussian kernel approximation
        const int radius = 2;

        for (int y = radius; y < NOISE_SIZE - radius; ++y) {
            for (int x = radius; x < NOISE_SIZE - radius; ++x) {
                int sum = 0;
                int count = 0;

                // Sample in a circular pattern with distance weighting
                for (int dy = -radius; dy <= radius; ++dy) {
                    for (int dx = -radius; dx <= radius; ++dx) {
                        // Weight by distance from center (approximate Gaussian)
                        int weight = (radius + 1) - qMax(qAbs(dx), qAbs(dy));
                        sum += m_noiseTexture.pixelColor(x + dx, y + dy).alpha() * weight;
                        count += weight;
                    }
                }

                int avgAlpha = sum / count;
                blurred.setPixelColor(x, y, QColor(255, 255, 255, avgAlpha));
            }
        }

        m_noiseTexture = blurred;
    }

    // Make the noise tileable by blending edges
    const int blendSize = NOISE_SIZE / 8;
    for (int y = 0; y < NOISE_SIZE; ++y) {
        for (int x = 0; x < blendSize; ++x) {
            qreal blend = static_cast<qreal>(x) / blendSize;
            int leftAlpha = m_noiseTexture.pixelColor(x, y).alpha();
            int rightAlpha = m_noiseTexture.pixelColor(NOISE_SIZE - blendSize + x, y).alpha();
            int blendedAlpha = static_cast<int>(leftAlpha * blend + rightAlpha * (1.0 - blend));
            m_noiseTexture.setPixelColor(x, y, QColor(255, 255, 255, blendedAlpha));
        }
    }
    for (int x = 0; x < NOISE_SIZE; ++x) {
        for (int y = 0; y < blendSize; ++y) {
            qreal blend = static_cast<qreal>(y) / blendSize;
            int topAlpha = m_noiseTexture.pixelColor(x, y).alpha();
            int bottomAlpha = m_noiseTexture.pixelColor(x, NOISE_SIZE - blendSize + y).alpha();
            int blendedAlpha = static_cast<int>(topAlpha * blend + bottomAlpha * (1.0 - blend));
            m_noiseTexture.setPixelColor(x, y, QColor(255, 255, 255, blendedAlpha));
        }
    }
}

void FogMistEffect::updateFogGradient()
{
    // Reserved for future gradient caching optimization
}

bool FogMistEffect::loadFogTexture(const QString& filePath)
{
    QPixmap texture(filePath);
    if (texture.isNull()) {
        DebugConsole::warning(
            QString("FogMistEffect: Failed to load texture from %1").arg(filePath),
            "Atmosphere");
        return false;
    }

    m_fogTexture = texture;
    m_hasTexture = true;
    m_lastTintColor = QColor();  // Reset tint to force re-tint

    DebugConsole::info(
        QString("FogMistEffect: Loaded fog texture %1 size: %2x%3")
            .arg(filePath)
            .arg(texture.width())
            .arg(texture.height()),
        "Atmosphere");

    update();
    return true;
}

void FogMistEffect::clearFogTexture()
{
    m_fogTexture = QPixmap();
    m_tintedTexture = QPixmap();
    m_hasTexture = false;

    // Clear cached scaled textures
    for (int i = 0; i < NUM_TEXTURE_LAYERS; ++i) {
        m_scaledTextures[i] = QPixmap();
        m_cachedTileSizes[i] = QSize();
    }

    update();
}

void FogMistEffect::setTextureScale(qreal scale)
{
    qreal newScale = qBound(0.1, scale, 10.0);
    if (!qFuzzyCompare(m_textureScale, newScale)) {
        m_textureScale = newScale;
        // Invalidate cached scaled textures since tile sizes will change
        for (int i = 0; i < NUM_TEXTURE_LAYERS; ++i) {
            m_cachedTileSizes[i] = QSize();  // Force rescale on next paint
        }
        update();
    }
}

void FogMistEffect::setTextureTwist(qreal twist)
{
    m_textureTwist = qBound(0.0, twist, 1.0);
    update();
}

void FogMistEffect::paintTextureLayer(QPainter* painter, const QRectF& fogRect, int baseAlpha)
{
    if (!m_hasTexture || m_fogTexture.isNull()) {
        return;
    }

    // Re-tint texture if color changed (and invalidate scaled cache)
    bool textureChanged = false;
    if (m_lastTintColor != m_color) {
        // Create tinted version of texture
        QImage tintedImg = m_fogTexture.toImage().convertToFormat(QImage::Format_ARGB32);

        for (int y = 0; y < tintedImg.height(); ++y) {
            QRgb* scanLine = reinterpret_cast<QRgb*>(tintedImg.scanLine(y));
            for (int x = 0; x < tintedImg.width(); ++x) {
                QRgb pixel = scanLine[x];
                int alpha = qAlpha(pixel);
                if (alpha > 0) {
                    // Blend original grayscale with tint color
                    int gray = qGray(pixel);
                    int r = (m_color.red() * gray) / 255;
                    int g = (m_color.green() * gray) / 255;
                    int b = (m_color.blue() * gray) / 255;
                    scanLine[x] = qRgba(r, g, b, alpha);
                }
            }
        }

        m_tintedTexture = QPixmap::fromImage(tintedImg);
        m_lastTintColor = m_color;
        textureChanged = true;  // Invalidate scaled texture cache
        DebugConsole::info(
            QString("FogMistEffect: Tinted texture to color %1").arg(m_color.name()),
            "Atmosphere");
    }

    const QPixmap& texToUse = m_tintedTexture.isNull() ? m_fogTexture : m_tintedTexture;

    // Set clipping to fog area
    painter->save();
    painter->setClipRect(fogRect);

    // ========================================================================
    // Render 3 texture layers at different scales, positions, and rotations
    // for a more organic, dynamic fog appearance
    // ========================================================================

    struct TextureLayer {
        qreal scale;       // Size multiplier
        qreal scrollSpeedX;
        qreal scrollSpeedY;
        qreal rotationSpeed;
        qreal rotationAmount;
        qreal opacity;
        qreal phaseOffset;  // For varied movement
    };

    // Define 3 layers with different characteristics
    const TextureLayer layers[] = {
        // Large, slow background layer
        { 0.8, 0.3, 0.2, 0.001, 8.0, 0.4, 0.0 },
        // Medium, medium-speed middle layer
        { 0.5, 0.6, 0.4, 0.0018, 12.0, 0.35, 2.1 },
        // Smaller, faster foreground wisps
        { 0.3, 1.0, 0.7, 0.003, 20.0, 0.25, 4.2 }
    };

    const int numLayers = NUM_TEXTURE_LAYERS;

    for (int layer = 0; layer < numLayers; ++layer) {
        const TextureLayer& L = layers[layer];

        // Calculate tile size for this layer
        qreal baseTileSize = qMin(fogRect.width(), fogRect.height()) / 3.0;
        qreal tileSize = baseTileSize * L.scale * m_textureScale;

        // Maintain aspect ratio
        qreal aspectRatio = static_cast<qreal>(texToUse.height()) / texToUse.width();
        int tileWidth = static_cast<int>(tileSize);
        int tileHeight = static_cast<int>(tileSize * aspectRatio);

        // OPTIMIZATION: Cache scaled textures - only rescale if size changed or texture changed
        QSize targetSize(tileWidth, tileHeight);
        if (textureChanged || m_cachedTileSizes[layer] != targetSize || m_scaledTextures[layer].isNull()) {
            m_scaledTextures[layer] = texToUse.scaled(
                tileWidth,
                tileHeight,
                Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation
            );
            m_cachedTileSizes[layer] = targetSize;
        }

        // Calculate scroll offset with layer-specific speeds
        qreal scrollX = m_animationOffset * L.scrollSpeedX;
        qreal scrollY = m_animationOffset * L.scrollSpeedY;

        // Add some sinusoidal undulation to the scroll for organic movement
        scrollX += qSin(m_animationOffset * 0.002 + L.phaseOffset) * fogRect.width() * 0.05;
        scrollY += qCos(m_animationOffset * 0.0015 + L.phaseOffset) * fogRect.height() * 0.05;

        // Calculate twist/rotation - more pronounced and varied per layer
        qreal twistAngle = qSin(m_animationOffset * L.rotationSpeed + L.phaseOffset)
                          * L.rotationAmount * m_textureTwist;

        // Add secondary rotation for more complex movement
        twistAngle += qCos(m_animationOffset * L.rotationSpeed * 0.7 + L.phaseOffset * 1.5)
                     * L.rotationAmount * m_textureTwist * 0.3;

        // Calculate layer opacity based on density
        qreal layerOpacity = m_density * L.opacity;

        painter->save();
        painter->setOpacity(layerOpacity);

        // Center rotation around fog center with slight offset per layer
        QPointF center = fogRect.center();
        qreal offsetX = qSin(m_animationOffset * 0.001 + layer * 2.0) * fogRect.width() * 0.1;
        qreal offsetY = qCos(m_animationOffset * 0.0008 + layer * 2.0) * fogRect.height() * 0.1;
        QPointF rotationCenter(center.x() + offsetX, center.y() + offsetY);

        painter->translate(rotationCenter);
        painter->rotate(twistAngle);
        painter->translate(-rotationCenter);

        // Calculate expanded bounds to cover rotation
        qreal expandFactor = 1.8;  // Extra coverage for rotation
        QRectF expandedRect(
            fogRect.x() - fogRect.width() * (expandFactor - 1) / 2,
            fogRect.y() - fogRect.height() * (expandFactor - 1) / 2,
            fogRect.width() * expandFactor,
            fogRect.height() * expandFactor
        );

        // Calculate scroll offset for tiling start position
        // Use fmod with positive adjustment to handle negative values
        qreal offsetModX = fmod(scrollX, static_cast<qreal>(tileWidth));
        if (offsetModX < 0) offsetModX += tileWidth;
        qreal offsetModY = fmod(scrollY, static_cast<qreal>(tileHeight));
        if (offsetModY < 0) offsetModY += tileHeight;

        // Use Qt's optimized drawTiledPixmap with cached scaled texture
        QPointF tileOffset(-offsetModX, -offsetModY);
        painter->drawTiledPixmap(expandedRect.toRect(), m_scaledTextures[layer], tileOffset.toPoint());

        painter->restore();
    }

    painter->restore();
}
