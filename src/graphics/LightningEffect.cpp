#include "LightningEffect.h"
#include "graphics/ZLayers.h"
#include <QPainter>
#include <QDateTime>
#include "utils/DebugConsole.h"

LightningEffect::LightningEffect(QGraphicsItem* parent)
    : QObject(nullptr)
    , QGraphicsItem(parent)
    , m_intensity(0.8)
    , m_frequency(0.5)
    , m_color(QColor(220, 230, 255))  // Slight blue-white for lightning
    , m_enabled(false)
    , m_isStriking(false)
    , m_strikePhase(0.0)
    , m_strikeCount(1)
    , m_currentFlash(0)
    , m_updateTimer(new QTimer(this))
    , m_strikeStartTime(0)
    , m_strikeTimer(new QTimer(this))
    , m_transitionTimer(new QTimer(this))
    , m_startIntensity(0.0)
    , m_targetIntensity(0.0)
    , m_startFrequency(0.0)
    , m_targetFrequency(0.0)
    , m_transitionStartTime(0)
    , m_transitionDuration(1000)
    , m_isTransitioning(false)
    , m_random(QRandomGenerator::global()->generate())
{
    setZValue(ZLayer::Lightning);

    // Setup update timer for flash animation
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &LightningEffect::onUpdateTick);

    // Setup strike scheduling timer (single-shot)
    m_strikeTimer->setSingleShot(true);
    connect(m_strikeTimer, &QTimer::timeout, this, &LightningEffect::onStrikeTick);

    // Setup transition timer
    m_transitionTimer->setInterval(TRANSITION_INTERVAL_MS);
    connect(m_transitionTimer, &QTimer::timeout, this, &LightningEffect::onTransitionTick);

    // Start invisible
    setVisible(false);
}

LightningEffect::~LightningEffect()
{
    if (m_updateTimer->isActive()) {
        m_updateTimer->stop();
    }
    if (m_strikeTimer->isActive()) {
        m_strikeTimer->stop();
    }
    if (m_transitionTimer->isActive()) {
        m_transitionTimer->stop();
    }
}

QRectF LightningEffect::boundingRect() const
{
    return m_sceneBounds;
}

void LightningEffect::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/,
                            QWidget* /*widget*/)
{
    if (!m_enabled || !m_isStriking || m_sceneBounds.isEmpty()) {
        return;
    }

    // Calculate flash opacity based on phase
    qreal opacity = calculateFlashOpacity();
    if (opacity <= 0.0) {
        return;
    }

    // Apply intensity scaling
    opacity *= m_intensity;

    // Create flash color with calculated opacity
    QColor flashColor = m_color;
    flashColor.setAlphaF(opacity);

    // Save painter state
    painter->save();

    // Fill the entire scene with the flash color
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter->fillRect(m_sceneBounds, flashColor);

    // Restore painter state
    painter->restore();
}

void LightningEffect::setIntensity(qreal intensity)
{
    m_intensity = qBound(0.0, intensity, 1.0);
    if (m_enabled) update();
}

void LightningEffect::setFrequency(qreal frequency)
{
    m_frequency = qBound(0.0, frequency, 1.0);

    // If enabled, reschedule next strike based on new frequency
    if (m_enabled && !m_isStriking) {
        scheduleNextStrike();
    }
}

void LightningEffect::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void LightningEffect::setSceneBounds(const QRectF& bounds)
{
    if (m_sceneBounds != bounds) {
        prepareGeometryChange();
        m_sceneBounds = bounds;
        update();
    }
}

void LightningEffect::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    setVisible(enabled);

    if (enabled) {
        // Schedule first strike
        scheduleNextStrike();
        DebugConsole::info(
            QString("LightningEffect: Enabled with intensity %1 frequency %2")
                .arg(m_intensity, 0, 'f', 2)
                .arg(m_frequency, 0, 'f', 2),
            "Atmosphere");
    } else {
        // Stop all timers
        m_updateTimer->stop();
        m_strikeTimer->stop();
        m_isStriking = false;
        DebugConsole::info("LightningEffect: Disabled", "Atmosphere");
    }
}

void LightningEffect::triggerStrike()
{
    if (m_enabled && !m_isStriking) {
        startStrike();
    }
}

void LightningEffect::transitionTo(qreal intensity, qreal frequency, int durationMs)
{
    // Cancel any existing transition
    if (m_isTransitioning) {
        m_transitionTimer->stop();
    }

    m_startIntensity = m_intensity;
    m_targetIntensity = qBound(0.0, intensity, 1.0);
    m_startFrequency = m_frequency;
    m_targetFrequency = qBound(0.0, frequency, 1.0);

    m_transitionDuration = qMax(100, durationMs);
    m_transitionStartTime = QDateTime::currentMSecsSinceEpoch();
    m_isTransitioning = true;

    m_transitionTimer->start();
}

void LightningEffect::advanceAnimation(qreal dt)
{
    if (!m_enabled) {
        return;
    }

    if (!m_isStriking) {
        return;
    }

    // Accumulate elapsed time from driver dt (convert to ms for existing logic)
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_strikeStartTime;

    // Multi-flash sequence handling (same logic as onUpdateTick)
    if (m_strikeCount > 1) {
        int timeInSequence = static_cast<int>(elapsed);

        for (int i = 0; i < m_strikeCount; ++i) {
            if (timeInSequence < FLASH_DURATION_MS) {
                m_currentFlash = i;
                m_strikePhase = static_cast<qreal>(timeInSequence) / FLASH_DURATION_MS;
                // Do NOT call update() — SceneAnimationDriver handles scene->update()
                return;
            }
            timeInSequence -= FLASH_DURATION_MS;

            if (i < m_strikeCount - 1) {
                if (timeInSequence < FLASH_GAP_MS) {
                    m_strikePhase = 0.0;
                    return;
                }
                timeInSequence -= FLASH_GAP_MS;
            }
        }

        // All flashes complete
        m_isStriking = false;
        m_strikePhase = 0.0;
        m_updateTimer->stop();  // Safety: stop legacy timer if running
        scheduleNextStrike();
        return;
    }

    // Single flash
    if (elapsed >= FLASH_DURATION_MS) {
        m_isStriking = false;
        m_strikePhase = 0.0;
        m_updateTimer->stop();  // Safety: stop legacy timer if running
        scheduleNextStrike();
        return;
    }

    m_strikePhase = static_cast<qreal>(elapsed) / FLASH_DURATION_MS;
    // Do NOT call update() — SceneAnimationDriver handles scene->update()
}

void LightningEffect::onUpdateTick()
{
    if (!m_isStriking) {
        m_updateTimer->stop();
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_strikeStartTime;

    // Multi-flash sequence handling
    if (m_strikeCount > 1) {
        // Multi-flash: account for gaps between flashes
        int timeInSequence = static_cast<int>(elapsed);

        for (int i = 0; i < m_strikeCount; ++i) {
            if (timeInSequence < FLASH_DURATION_MS) {
                m_currentFlash = i;
                m_strikePhase = static_cast<qreal>(timeInSequence) / FLASH_DURATION_MS;
                update();
                return;
            }
            timeInSequence -= FLASH_DURATION_MS;

            // Gap between flashes (except after last flash)
            if (i < m_strikeCount - 1) {
                if (timeInSequence < FLASH_GAP_MS) {
                    // In the gap - no flash
                    m_strikePhase = 0.0;
                    update();
                    return;
                }
                timeInSequence -= FLASH_GAP_MS;
            }
        }

        // All flashes complete
        m_isStriking = false;
        m_strikePhase = 0.0;
        m_updateTimer->stop();
        update();

        // Schedule next strike
        scheduleNextStrike();
        return;
    }

    // Single flash
    if (elapsed >= FLASH_DURATION_MS) {
        m_isStriking = false;
        m_strikePhase = 0.0;
        m_updateTimer->stop();
        update();

        // Schedule next strike
        scheduleNextStrike();
        return;
    }

    m_strikePhase = static_cast<qreal>(elapsed) / FLASH_DURATION_MS;
    update();
}

void LightningEffect::onStrikeTick()
{
    if (m_enabled && !m_isStriking) {
        startStrike();
    }
}

void LightningEffect::onTransitionTick()
{
    if (!m_isTransitioning) {
        m_transitionTimer->stop();
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_transitionStartTime;
    qreal progress = qBound(0.0, static_cast<qreal>(elapsed) / m_transitionDuration, 1.0);

    // Linear interpolation
    m_intensity = m_startIntensity + (m_targetIntensity - m_startIntensity) * progress;
    m_frequency = m_startFrequency + (m_targetFrequency - m_startFrequency) * progress;

    if (progress >= 1.0) {
        m_intensity = m_targetIntensity;
        m_frequency = m_targetFrequency;
        m_isTransitioning = false;
        m_transitionTimer->stop();
        emit transitionCompleted();
    }

    update();
}

void LightningEffect::startStrike()
{
    if (!m_enabled) {
        return;
    }

    m_isStriking = true;
    m_strikeStartTime = QDateTime::currentMSecsSinceEpoch();
    m_strikePhase = 0.0;
    m_currentFlash = 0;

    // Randomly determine number of flashes (1-3, weighted towards 1-2)
    int rand = m_random.bounded(100);
    if (rand < 50) {
        m_strikeCount = 1;  // 50% chance of single flash
    } else if (rand < 85) {
        m_strikeCount = 2;  // 35% chance of double flash
    } else {
        m_strikeCount = 3;  // 15% chance of triple flash
    }

    // Update timer no longer auto-started — SceneAnimationDriver calls advanceAnimation()
    // (m_updateTimer kept as fallback but not started here)

    // Emit signal for potential sound effects
    emit lightningStrike();
}

void LightningEffect::scheduleNextStrike()
{
    if (!m_enabled || m_frequency <= 0.0) {
        return;
    }

    // Calculate interval based on frequency
    // At frequency 0.0: no strikes
    // At frequency 0.5: strikes every ~9 seconds on average
    // At frequency 1.0: strikes every ~3 seconds on average
    qreal intervalRange = MAX_STRIKE_INTERVAL_MS - MIN_STRIKE_INTERVAL_MS;
    qreal baseInterval = MAX_STRIKE_INTERVAL_MS - (intervalRange * m_frequency);

    // Add some randomness (+-30%)
    qreal randomFactor = 0.7 + (m_random.bounded(60) / 100.0);  // 0.7 to 1.3
    int interval = static_cast<int>(baseInterval * randomFactor);

    m_strikeTimer->start(interval);
}

qreal LightningEffect::calculateFlashOpacity() const
{
    if (!m_isStriking) {
        return 0.0;
    }

    // Flash profile: quick rise, slower fade
    // 0.0 -> 0.2: rapid rise to full brightness
    // 0.2 -> 1.0: gradual fade out

    if (m_strikePhase < 0.2) {
        // Quick rise
        return m_strikePhase / 0.2;
    } else {
        // Fade out with slight curve
        qreal fadePhase = (m_strikePhase - 0.2) / 0.8;
        return 1.0 - (fadePhase * fadePhase);  // Quadratic ease-out
    }
}
