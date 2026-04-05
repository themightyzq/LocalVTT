#include "WeatherEffect.h"
#include "graphics/ZLayers.h"
#include <QPainter>
#include <QDateTime>
#include <QRandomGenerator>
#include <QtMath>
#include "utils/DebugConsole.h"

WeatherEffect::WeatherEffect(QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_weatherType(WeatherType::None)
    , m_intensity(0.5)
    , m_windStrength(0.0)
    , m_enabled(false)
    , m_sceneScale(1.0)
    , m_updateTimer(new QTimer(this))
    , m_lastUpdateTime(0)
    , m_transitionTimer(new QTimer(this))
    , m_targetType(WeatherType::None)
    , m_startIntensity(0.0)
    , m_targetIntensity(0.0)
    , m_transitionStartTime(0)
    , m_transitionDuration(1000)
    , m_isTransitioning(false)
{
    setZValue(ZLayer::Weather);

    // Reserve particle pool
    m_particles.reserve(MAX_PARTICLES);

    // Setup update timer
    connect(m_updateTimer, &QTimer::timeout, this, &WeatherEffect::onUpdateTick);

    // Setup transition timer
    connect(m_transitionTimer, &QTimer::timeout, this, &WeatherEffect::onTransitionTick);

    // Initialize last update time
    m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
}

WeatherEffect::~WeatherEffect()
{
    if (m_updateTimer->isActive()) {
        m_updateTimer->stop();
    }
    if (m_transitionTimer->isActive()) {
        m_transitionTimer->stop();
    }
}

QRectF WeatherEffect::boundingRect() const
{
    // Return scene bounds if set, otherwise a default large rect
    if (m_sceneBounds.isValid()) {
        return m_sceneBounds;
    }
    return QRectF(-5000, -5000, 10000, 10000);
}

void WeatherEffect::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/,
                          QWidget* /*widget*/)
{
    if (!m_enabled || m_weatherType == WeatherType::None || m_intensity <= 0.0) {
        return;
    }

    painter->save();

    // Enable antialiasing for rain (lines benefit from it)
    // Snow uses drawRects with AA off for performance
    bool useAA = (m_weatherType == WeatherType::Rain || m_weatherType == WeatherType::Storm);
    painter->setRenderHint(QPainter::Antialiasing, useAA);

    switch (m_weatherType) {
        case WeatherType::Rain:
        case WeatherType::Storm:
            paintRain(painter);
            break;
        case WeatherType::Snow:
            paintSnow(painter);
            break;
        default:
            break;
    }

    painter->restore();
}

void WeatherEffect::setWeatherType(WeatherType type)
{
    if (m_weatherType != type) {
        m_weatherType = type;
        initializeParticles();
        update();
    }
}

void WeatherEffect::setIntensity(qreal intensity)
{
    m_intensity = qBound(0.0, intensity, 1.0);
    if (m_enabled) update();
}

void WeatherEffect::setWindStrength(qreal strength)
{
    m_windStrength = qBound(-1.0, strength, 1.0);
}

void WeatherEffect::setSceneBounds(const QRectF& bounds)
{
    if (m_sceneBounds != bounds) {
        prepareGeometryChange();
        m_sceneBounds = bounds;

        // Calculate scale factor based on scene size
        // Base reference: 1000x1000 scene = scale 1.0
        // Larger scenes get larger particles to remain visible
        qreal avgDimension = (bounds.width() + bounds.height()) / 2.0;
        m_sceneScale = qMax(1.0, avgDimension / 1000.0);

        DebugConsole::info(
            QString("WeatherEffect: Scene bounds set to %1x%2 scale: %3")
                .arg(bounds.width(), 0, 'f', 0)
                .arg(bounds.height(), 0, 'f', 0)
                .arg(m_sceneScale, 0, 'f', 2),
            "Weather");

        initializeParticles();
    }
}

void WeatherEffect::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;

        if (enabled && m_weatherType != WeatherType::None) {
            initializeParticles();
            m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
            // Timer no longer auto-started — SceneAnimationDriver calls advanceAnimation()
        } else {
            m_updateTimer->stop();
            m_particles.clear();
        }

        update();
    }
}

void WeatherEffect::transitionTo(WeatherType type, qreal intensity, int durationMs)
{
    if (type == m_weatherType && qFuzzyCompare(intensity, m_intensity)) {
        return;
    }

    m_targetType = type;
    m_startIntensity = m_intensity;
    m_targetIntensity = intensity;
    m_transitionDuration = durationMs;
    m_transitionStartTime = QDateTime::currentMSecsSinceEpoch();
    m_isTransitioning = true;

    // If changing type, set it immediately but start at 0 intensity
    if (type != m_weatherType) {
        m_weatherType = type;
        m_intensity = 0.0;
        initializeParticles();
    }

    // Ensure effect is enabled during transition
    if (!m_enabled && type != WeatherType::None) {
        setEnabled(true);
    }

    m_transitionTimer->start(TRANSITION_INTERVAL_MS);
}

void WeatherEffect::advanceAnimation(qreal dt)
{
    if (!m_enabled || m_weatherType == WeatherType::None) {
        return;
    }

    // dt is already capped by SceneAnimationDriver
    updateParticles(dt);
    // Do NOT call update() — SceneAnimationDriver handles scene->update()
}

void WeatherEffect::onUpdateTick()
{
    if (!m_enabled || m_weatherType == WeatherType::None) {
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qreal deltaTime = (currentTime - m_lastUpdateTime) / 1000.0;  // Convert to seconds
    m_lastUpdateTime = currentTime;

    // Cap delta time to prevent huge jumps
    deltaTime = qMin(deltaTime, 0.1);

    updateParticles(deltaTime);
    update();  // Request repaint
}

void WeatherEffect::onTransitionTick()
{
    if (!m_isTransitioning) {
        m_transitionTimer->stop();
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_transitionStartTime;
    qreal progress = qBound(0.0, static_cast<qreal>(elapsed) / m_transitionDuration, 1.0);

    // Smooth easing
    qreal easedProgress = 0.5 - 0.5 * qCos(progress * M_PI);

    m_intensity = m_startIntensity + (m_targetIntensity - m_startIntensity) * easedProgress;

    if (progress >= 1.0) {
        m_isTransitioning = false;
        m_transitionTimer->stop();
        m_intensity = m_targetIntensity;

        // Disable if transitioning to none or zero intensity
        if (m_targetType == WeatherType::None || m_targetIntensity <= 0.0) {
            setEnabled(false);
        }

        emit transitionCompleted();
        DebugConsole::info(
            QString("WeatherEffect: Transition complete - type: %1 intensity: %2")
                .arg(static_cast<int>(m_weatherType))
                .arg(m_intensity, 0, 'f', 2),
            "Weather");
    }

    update();
}

void WeatherEffect::initializeParticles()
{
    m_particles.clear();

    if (m_weatherType == WeatherType::None || !m_sceneBounds.isValid()) {
        return;
    }

    // Calculate particle count based on intensity and scene size
    qreal area = m_sceneBounds.width() * m_sceneBounds.height();
    qreal densityFactor = area / (1000.0 * 1000.0);  // Normalize to 1000x1000
    int particleCount = static_cast<int>(MAX_PARTICLES * m_intensity * qMin(densityFactor, 1.0));
    particleCount = qBound(10, particleCount, MAX_PARTICLES);

    m_particles.resize(particleCount);

    // Initialize each particle
    for (int i = 0; i < particleCount; ++i) {
        spawnParticle(m_particles[i]);
        // Randomize initial position throughout the scene
        m_particles[i].position.setY(
            m_sceneBounds.top() +
            QRandomGenerator::global()->generateDouble() * m_sceneBounds.height()
        );
    }
}

void WeatherEffect::updateParticles(qreal deltaTime)
{
    if (!m_sceneBounds.isValid()) {
        return;
    }

    for (auto& particle : m_particles) {
        // Apply wind to velocity (scaled)
        qreal windEffect = m_windStrength * 200.0 * m_sceneScale;

        // Update position
        particle.position += particle.velocity * deltaTime;
        particle.position.rx() += windEffect * deltaTime;

        // Add wobble for snow (scaled)
        if (m_weatherType == WeatherType::Snow) {
            qreal wobble = qSin(particle.lifetime * 3.0) * m_snowSettings.wobbleAmount * m_sceneScale;
            particle.position.rx() += wobble * deltaTime;
            particle.lifetime += deltaTime;
        }

        // Check if particle is out of bounds (scaled margin)
        qreal margin = 50.0 * m_sceneScale;
        bool outOfBounds = false;
        if (particle.position.y() > m_sceneBounds.bottom()) {
            outOfBounds = true;
        } else if (particle.position.x() < m_sceneBounds.left() - margin ||
                   particle.position.x() > m_sceneBounds.right() + margin) {
            outOfBounds = true;
        }

        if (outOfBounds) {
            spawnParticle(particle);
        }
    }
}

void WeatherEffect::spawnParticle(WeatherParticle& particle)
{
    if (!m_sceneBounds.isValid()) {
        return;
    }

    auto* rng = QRandomGenerator::global();

    // Spawn at top of scene with random X position
    particle.position.setX(
        m_sceneBounds.left() + rng->generateDouble() * m_sceneBounds.width()
    );
    particle.position.setY(m_sceneBounds.top() - 10 * m_sceneScale);

    switch (m_weatherType) {
        case WeatherType::Rain:
        case WeatherType::Storm: {
            // Scale speed and size based on scene scale
            qreal baseSpeed = m_rainSettings.minSpeed +
                         rng->generateDouble() * (m_rainSettings.maxSpeed - m_rainSettings.minSpeed);
            qreal speed = baseSpeed * m_sceneScale;

            // Storm has more horizontal movement
            qreal horizontalSpeed = (m_weatherType == WeatherType::Storm) ?
                                   (rng->generateDouble() - 0.5) * 200.0 * m_sceneScale : 0.0;

            particle.velocity = QPointF(horizontalSpeed, speed);

            // Scale rain length
            qreal baseSize = m_rainSettings.minLength +
                           rng->generateDouble() * (m_rainSettings.maxLength - m_rainSettings.minLength);
            particle.size = baseSize * m_sceneScale;
            particle.opacity = 0.3 + rng->generateDouble() * 0.7;
            break;
        }

        case WeatherType::Snow: {
            // Scale speed based on scene scale
            qreal baseSpeed = m_snowSettings.minSpeed +
                         rng->generateDouble() * (m_snowSettings.maxSpeed - m_snowSettings.minSpeed);
            qreal speed = baseSpeed * m_sceneScale;
            particle.velocity = QPointF(0, speed);

            // Scale snowflake size
            qreal baseSize = m_snowSettings.minSize +
                           rng->generateDouble() * (m_snowSettings.maxSize - m_snowSettings.minSize);
            particle.size = baseSize * m_sceneScale;
            particle.opacity = 0.5 + rng->generateDouble() * 0.5;
            particle.lifetime = rng->generateDouble() * 10.0;  // Random phase for wobble
            break;
        }

        default:
            break;
    }
}

void WeatherEffect::paintRain(QPainter* painter)
{
    if (m_particles.isEmpty()) return;

    QColor rainColor = m_rainSettings.color;
    rainColor.setAlphaF(rainColor.alphaF() * m_intensity);

    // Scale pen width based on scene scale
    qreal scaledWidth = m_rainSettings.width * m_sceneScale;

    // OPTIMIZATION: Batch particles by opacity bucket to minimize QPen changes
    // Use 10 buckets (0.0-0.1, 0.1-0.2, ... 0.9-1.0)
    constexpr int NUM_BUCKETS = 10;
    QVector<QLineF> buckets[NUM_BUCKETS];

    // Pre-allocate bucket capacity
    int avgPerBucket = m_particles.size() / NUM_BUCKETS + 1;
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        buckets[i].reserve(avgPerBucket);
    }

    // Sort particles into opacity buckets and pre-compute lines
    for (const auto& particle : m_particles) {
        int bucket = qBound(0, static_cast<int>(particle.opacity * NUM_BUCKETS), NUM_BUCKETS - 1);

        // Pre-compute line direction
        qreal len = std::sqrt(particle.velocity.x() * particle.velocity.x() +
                              particle.velocity.y() * particle.velocity.y());
        QPointF dir = (len > 0.001) ? (particle.velocity / len) * particle.size : QPointF(0, 1) * particle.size;

        buckets[bucket].append(QLineF(particle.position, particle.position + dir));
    }

    // Draw each bucket with a single pen
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        if (buckets[i].isEmpty()) continue;

        qreal bucketOpacity = (i + 0.5) / NUM_BUCKETS;  // Mid-point of bucket
        QColor bucketColor = rainColor;
        bucketColor.setAlphaF(rainColor.alphaF() * bucketOpacity);

        painter->setPen(QPen(bucketColor, scaledWidth, Qt::SolidLine, Qt::RoundCap));
        painter->drawLines(buckets[i]);  // Batch draw all lines in bucket
    }
}

void WeatherEffect::paintSnow(QPainter* painter)
{
    if (m_particles.isEmpty()) return;

    painter->setPen(Qt::NoPen);

    // OPTIMIZATION: Batch particles into opacity buckets, draw all rects per bucket
    // in a single drawRects() call. ~5 draw calls instead of ~500 drawEllipse calls.
    // Antialiasing is disabled for snow (set in paint()) — squares look fine at this scale.
    constexpr int NUM_BUCKETS = 5;

    QVector<QRectF> buckets[NUM_BUCKETS];

    // Pre-allocate bucket capacity
    int avgPerBucket = m_particles.size() / NUM_BUCKETS + 1;
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        buckets[i].reserve(avgPerBucket);
    }

    // Sort particles into opacity buckets, building QRectF directly
    for (const auto& particle : m_particles) {
        int bucket = qBound(0, static_cast<int>(particle.opacity * NUM_BUCKETS), NUM_BUCKETS - 1);
        buckets[bucket].append(QRectF(
            particle.position.x() - particle.size,
            particle.position.y() - particle.size,
            particle.size * 2.0,
            particle.size * 2.0));
    }

    // Draw each bucket with a single brush + single drawRects call
    QColor snowColor = m_snowSettings.color;
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        if (buckets[i].isEmpty()) continue;

        qreal bucketOpacity = (i + 0.5) / NUM_BUCKETS;  // Mid-point of bucket
        QColor bucketColor = snowColor;
        bucketColor.setAlphaF(snowColor.alphaF() * m_intensity * bucketOpacity);

        painter->setBrush(bucketColor);
        painter->drawRects(buckets[i].constData(), buckets[i].size());
    }
}
