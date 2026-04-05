#ifndef WEATHEREFFECT_H
#define WEATHEREFFECT_H

#include <QGraphicsItem>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QColor>

// Weather type enumeration
enum class WeatherType {
    None = 0,
    Rain = 1,
    Snow = 2,
    Storm = 3  // Heavy rain with wind
};

// Individual particle data
struct WeatherParticle {
    QPointF position;
    QPointF velocity;
    qreal size;
    qreal opacity;
    qreal lifetime;  // For snow fade-in/out
};

// Weather particle effect overlay
// Z-value: 50 (per CLAUDE.md: 40-99 reserved for atmosphere overlays)
class WeatherEffect : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit WeatherEffect(QGraphicsItem* parent = nullptr);
    ~WeatherEffect() override;

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    // Weather control
    void setWeatherType(WeatherType type);
    WeatherType getWeatherType() const { return m_weatherType; }

    void setIntensity(qreal intensity);  // 0.0 to 1.0
    qreal getIntensity() const { return m_intensity; }

    void setWindStrength(qreal strength);  // -1.0 to 1.0 (negative = left, positive = right)
    qreal getWindStrength() const { return m_windStrength; }

    // Scene bounds - must be set for particles to know where to spawn/despawn
    void setSceneBounds(const QRectF& bounds);
    QRectF getSceneBounds() const { return m_sceneBounds; }

    // Enable/disable
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Transition support - smooth intensity changes
    void transitionTo(WeatherType type, qreal intensity, int durationMs = 1000);

public slots:
    void advanceAnimation(qreal dt);

signals:
    void transitionCompleted();

private slots:
    void onUpdateTick();
    void onTransitionTick();

private:
    void initializeParticles();
    void updateParticles(qreal deltaTime);
    void spawnParticle(WeatherParticle& particle);
    void paintRain(QPainter* painter);
    void paintSnow(QPainter* painter);

    // Weather state
    WeatherType m_weatherType;
    qreal m_intensity;      // 0.0 to 1.0
    qreal m_windStrength;   // -1.0 to 1.0
    bool m_enabled;

    // Particle pool
    QVector<WeatherParticle> m_particles;
    static constexpr int MAX_PARTICLES = 500;

    // Scene bounds for particle spawning
    QRectF m_sceneBounds;
    qreal m_sceneScale;  // Scale factor based on scene size

    // Animation timer (30 FPS per plan)
    QTimer* m_updateTimer;
    qint64 m_lastUpdateTime;

    // Transition support
    QTimer* m_transitionTimer;
    WeatherType m_targetType;
    qreal m_startIntensity;
    qreal m_targetIntensity;
    qint64 m_transitionStartTime;
    int m_transitionDuration;
    bool m_isTransitioning;

    // Visual settings per weather type
    struct RainSettings {
        QColor color = QColor(180, 200, 255, 150);
        qreal minLength = 15.0;
        qreal maxLength = 30.0;
        qreal minSpeed = 800.0;
        qreal maxSpeed = 1200.0;
        qreal width = 1.5;
    } m_rainSettings;

    struct SnowSettings {
        QColor color = QColor(255, 255, 255, 200);
        qreal minSize = 2.0;
        qreal maxSize = 6.0;
        qreal minSpeed = 50.0;
        qreal maxSpeed = 150.0;
        qreal wobbleAmount = 30.0;  // Horizontal drift
    } m_snowSettings;

    // Timer intervals
    static constexpr int UPDATE_INTERVAL_MS = 33;  // ~30 FPS
    static constexpr int TRANSITION_INTERVAL_MS = 16;  // 60 FPS for smooth transitions

};

#endif // WEATHEREFFECT_H
