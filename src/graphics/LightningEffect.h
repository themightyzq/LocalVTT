#ifndef LIGHTNINGEFFECT_H
#define LIGHTNINGEFFECT_H

#include <QGraphicsItem>
#include <QTimer>
#include <QColor>
#include <QRandomGenerator>

// Lightning flash overlay effect
// Z-value: 70 (per CLAUDE.md: between WeatherEffect at 50 and Tool previews at 100)
class LightningEffect : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit LightningEffect(QGraphicsItem* parent = nullptr);
    ~LightningEffect() override;

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    // Lightning control
    void setIntensity(qreal intensity);  // 0.0 to 1.0 - maximum brightness of flash
    qreal getIntensity() const { return m_intensity; }

    void setFrequency(qreal frequency);  // 0.0 to 1.0 - how often strikes occur
    qreal getFrequency() const { return m_frequency; }

    void setColor(const QColor& color);
    QColor getColor() const { return m_color; }

    // Scene bounds - defines the flash coverage area
    void setSceneBounds(const QRectF& bounds);
    QRectF getSceneBounds() const { return m_sceneBounds; }

    // Enable/disable
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Force a lightning strike (for testing or manual triggering)
    void triggerStrike();

    // Transition support - smooth intensity changes
    void transitionTo(qreal intensity, qreal frequency, int durationMs = 1000);

public slots:
    void advanceAnimation(qreal dt);

signals:
    void transitionCompleted();
    void lightningStrike();  // Emitted when a strike occurs (for sound effects)

private slots:
    void onUpdateTick();
    void onStrikeTick();
    void onTransitionTick();

private:
    void startStrike();
    void scheduleNextStrike();
    qreal calculateFlashOpacity() const;

    // Lightning state
    qreal m_intensity;      // 0.0 to 1.0 - max flash brightness
    qreal m_frequency;      // 0.0 to 1.0 - strike frequency
    QColor m_color;         // Flash color (default: white-blue)
    bool m_enabled;

    // Scene bounds for flash coverage
    QRectF m_sceneBounds;

    // Strike animation state
    bool m_isStriking;
    qreal m_strikePhase;    // 0.0 to 1.0 during a strike
    int m_strikeCount;      // Multiple flashes per strike (1-3)
    int m_currentFlash;     // Which flash in the sequence

    // Animation timer (60 FPS for smooth flash)
    QTimer* m_updateTimer;
    qint64 m_strikeStartTime;

    // Strike scheduling timer
    QTimer* m_strikeTimer;

    // Transition support
    QTimer* m_transitionTimer;
    qreal m_startIntensity;
    qreal m_targetIntensity;
    qreal m_startFrequency;
    qreal m_targetFrequency;
    qint64 m_transitionStartTime;
    int m_transitionDuration;
    bool m_isTransitioning;

    // Strike timing constants
    static constexpr int FLASH_DURATION_MS = 150;      // Duration of one flash
    static constexpr int FLASH_GAP_MS = 80;            // Gap between multi-flashes
    static constexpr int MIN_STRIKE_INTERVAL_MS = 3000;  // Minimum time between strikes
    static constexpr int MAX_STRIKE_INTERVAL_MS = 15000; // Maximum time between strikes

    // Timer intervals
    static constexpr int UPDATE_INTERVAL_MS = 16;       // ~60 FPS
    static constexpr int TRANSITION_INTERVAL_MS = 16;   // 60 FPS for smooth transitions


    // Random generator for timing
    QRandomGenerator m_random;
};

#endif // LIGHTNINGEFFECT_H
