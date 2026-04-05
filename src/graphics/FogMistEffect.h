#ifndef FOGMISTEFFECT_H
#define FOGMISTEFFECT_H

#include <QGraphicsItem>
#include <QTimer>
#include <QColor>
#include <QImage>
#include <QPixmap>

// Animated fog/mist overlay effect
// Z-value: 40 (per CLAUDE.md: 40-99 reserved for atmosphere overlays)
class FogMistEffect : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit FogMistEffect(QGraphicsItem* parent = nullptr);
    ~FogMistEffect() override;

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    // Fog control
    void setDensity(qreal density);  // 0.0 to 1.0
    qreal getDensity() const { return m_density; }

    void setHeight(qreal height);  // 0.0 to 1.0 (fraction of scene covered from bottom)
    qreal getHeight() const { return m_height; }

    void setColor(const QColor& color);
    QColor getColor() const { return m_color; }

    // Scene bounds - must be set for fog to know where to render
    void setSceneBounds(const QRectF& bounds);
    QRectF getSceneBounds() const { return m_sceneBounds; }

    // Enable/disable
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Animation speed (0.0 = static, 1.0 = fast)
    void setAnimationSpeed(qreal speed);
    qreal getAnimationSpeed() const { return m_animationSpeed; }

    // Texture-based fog layer
    bool loadFogTexture(const QString& filePath);
    void clearFogTexture();
    bool hasTexture() const { return m_hasTexture; }
    void setTextureScale(qreal scale);  // How large each tile appears (default 1.0)
    qreal getTextureScale() const { return m_textureScale; }
    void setTextureTwist(qreal twist);  // Amount of rotation/twist animation (0.0-1.0)
    qreal getTextureTwist() const { return m_textureTwist; }

    // Transition support
    void transitionTo(qreal density, qreal height, const QColor& color, int durationMs = 1000);

public slots:
    void advanceAnimation(qreal dt);

signals:
    void transitionCompleted();

private slots:
    void onAnimationTick();
    void onTransitionTick();

private:
    void generateNoiseTexture();
    void updateFogGradient();
    void paintTextureLayer(QPainter* painter, const QRectF& fogRect, int baseAlpha);

    // Fog state
    qreal m_density;        // 0.0 to 1.0
    qreal m_height;         // 0.0 to 1.0
    QColor m_color;
    bool m_enabled;
    qreal m_animationSpeed;

    // Scene bounds
    QRectF m_sceneBounds;
    qreal m_sceneScale;

    // Animation
    QTimer* m_animationTimer;
    qreal m_animationOffset;  // Current animation phase
    qint64 m_lastAnimationTime;

    // Noise texture for organic fog movement
    QImage m_noiseTexture;
    static constexpr int NOISE_SIZE = 512;  // Larger for smoother appearance on big maps

    // Pre-rendered fog frame (avoids expensive rendering in paint())
    QPixmap m_renderedFrame;
    bool m_frameValid = false;
    void renderFrame();

    // Custom fog texture (seamless tile)
    QPixmap m_fogTexture;
    QPixmap m_tintedTexture;  // Pre-tinted version for faster rendering
    bool m_hasTexture;
    qreal m_textureScale;     // Scale factor for texture tiles
    qreal m_textureTwist;     // Twist/rotation animation intensity
    QColor m_lastTintColor;   // Track color for re-tinting

    // Cached scaled textures for each layer (performance optimization)
    // Avoids expensive texture scaling on every paint call
    static constexpr int NUM_TEXTURE_LAYERS = 3;
    QPixmap m_scaledTextures[NUM_TEXTURE_LAYERS];
    QSize m_cachedTileSizes[NUM_TEXTURE_LAYERS];  // Track sizes to detect when rescale needed

    // Transition support
    QTimer* m_transitionTimer;
    qreal m_startDensity;
    qreal m_targetDensity;
    qreal m_startHeight;
    qreal m_targetHeight;
    QColor m_startColor;
    QColor m_targetColor;
    qint64 m_transitionStartTime;
    int m_transitionDuration;
    bool m_isTransitioning;

    // Timer intervals - optimized for performance
    static constexpr int ANIMATION_INTERVAL_MS = 100;  // 10 FPS for fog animation (reduced for performance)
    static constexpr int TRANSITION_INTERVAL_MS = 33;  // 30 FPS for smooth transitions

};

#endif // FOGMISTEFFECT_H
