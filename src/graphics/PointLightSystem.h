#ifndef POINTLIGHTSYSTEM_H
#define POINTLIGHTSYSTEM_H

#include <QGraphicsItem>
#include <QObject>
#include <QTimer>
#include <QList>
#include <QHash>
#include "PointLight.h"

class MapDisplay;

// Renders all point lights as radial gradient overlays
// Uses additive blending to brighten areas within light radius
// Z-value: 35 (between beacons and fog effects, per CLAUDE.md)
class PointLightSystem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit PointLightSystem(QGraphicsItem* parent = nullptr);
    ~PointLightSystem() override;

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    // Scene bounds - must be set for lights to render
    void setSceneBounds(const QRectF& bounds);
    QRectF getSceneBounds() const { return m_sceneBounds; }

    // Enable/disable the entire light system
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Light management
    QUuid addLight(const PointLight& light);
    QUuid addLightAtPosition(const QPointF& pos, LightPreset preset = LightPreset::Torch);
    void removeLight(const QUuid& id);
    void removeAllLights();
    void updateLight(const QUuid& id, const PointLight& light);
    PointLight* getLight(const QUuid& id);
    const PointLight* getLight(const QUuid& id) const;
    QList<PointLight> getAllLights() const { return m_lights.values(); }
    int lightCount() const { return m_lights.size(); }

    // Find light at position (for selection)
    QUuid lightAtPosition(const QPointF& pos) const;

    // Toggle individual light
    void toggleLight(const QUuid& id);

    // Global intensity multiplier (works with atmosphere system)
    void setGlobalIntensity(qreal intensity);
    qreal getGlobalIntensity() const { return m_globalIntensity; }

    // Ambient darkness level (0 = no darkness, 1 = completely dark except lights)
    void setAmbientDarkness(qreal darkness);
    qreal getAmbientDarkness() const { return m_ambientDarkness; }

public slots:
    void advanceAnimation(qreal dt);

signals:
    void lightAdded(const QUuid& id);
    void lightRemoved(const QUuid& id);
    void lightUpdated(const QUuid& id);
    void lightsChanged();

private slots:
    void onFlickerTick();

private:
    void paintLight(QPainter* painter, const PointLight& light, qreal flickerMod);

    // Light storage
    QHash<QUuid, PointLight> m_lights;

    // Scene bounds
    QRectF m_sceneBounds;

    // State
    bool m_enabled;
    qreal m_globalIntensity;
    qreal m_ambientDarkness;

    // Flicker animation
    QTimer* m_flickerTimer;
    qreal m_flickerPhase;
    qint64 m_lastFlickerTime;

    // Timer intervals
    static constexpr int FLICKER_INTERVAL_MS = 50;  // 20 FPS for flicker

};

#endif // POINTLIGHTSYSTEM_H
