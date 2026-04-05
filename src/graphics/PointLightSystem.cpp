#include "PointLightSystem.h"
#include "graphics/ZLayers.h"
#include <QPainter>
#include <QRadialGradient>
#include <QDateTime>
#include <QtMath>
#include "utils/DebugConsole.h"

PointLightSystem::PointLightSystem(QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_enabled(true)
    , m_globalIntensity(1.0)
    , m_ambientDarkness(0.7)  // Default: fairly dark, lights are visible
    , m_flickerTimer(new QTimer(this))
    , m_flickerPhase(0.0)
    , m_lastFlickerTime(0)
{
    setZValue(ZLayer::PointLights);

    // Setup flicker animation timer (kept as fallback, but not auto-started)
    connect(m_flickerTimer, &QTimer::timeout, this, &PointLightSystem::onFlickerTick);
    m_flickerTimer->setInterval(FLICKER_INTERVAL_MS);
    // Timer no longer auto-started — SceneAnimationDriver calls advanceAnimation()

    m_lastFlickerTime = QDateTime::currentMSecsSinceEpoch();
}

PointLightSystem::~PointLightSystem()
{
    if (m_flickerTimer->isActive()) {
        m_flickerTimer->stop();
    }
}

QRectF PointLightSystem::boundingRect() const
{
    return m_sceneBounds;
}

void PointLightSystem::setSceneBounds(const QRectF& bounds)
{
    if (m_sceneBounds != bounds) {
        prepareGeometryChange();
        m_sceneBounds = bounds;
        update();
    }
}

void PointLightSystem::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        setVisible(enabled);
        update();
    }
}

void PointLightSystem::setGlobalIntensity(qreal intensity)
{
    m_globalIntensity = qBound(0.0, intensity, 2.0);
    if (m_enabled) update();
}

void PointLightSystem::setAmbientDarkness(qreal darkness)
{
    m_ambientDarkness = qBound(0.0, darkness, 1.0);
    if (m_enabled) update();
}

QUuid PointLightSystem::addLight(const PointLight& light)
{
    m_lights.insert(light.id, light);
    emit lightAdded(light.id);
    emit lightsChanged();
    update();
    return light.id;
}

QUuid PointLightSystem::addLightAtPosition(const QPointF& pos, LightPreset preset)
{
    PointLight light;

    switch (preset) {
        case LightPreset::Torch:
            light = PointLight::torch(pos);
            break;
        case LightPreset::Lantern:
            light = PointLight::lantern(pos);
            break;
        case LightPreset::Campfire:
            light = PointLight::campfire(pos);
            break;
        case LightPreset::Magical:
            light = PointLight::magical(pos);
            break;
        case LightPreset::Moonbeam:
            light = PointLight::moonbeam(pos);
            break;
        case LightPreset::Custom:
        default:
            light = PointLight(pos);
            break;
    }

    return addLight(light);
}

void PointLightSystem::removeLight(const QUuid& id)
{
    if (m_lights.remove(id)) {
        emit lightRemoved(id);
        emit lightsChanged();
        update();
    }
}

void PointLightSystem::removeAllLights()
{
    m_lights.clear();
    emit lightsChanged();
    update();
}

void PointLightSystem::updateLight(const QUuid& id, const PointLight& light)
{
    if (m_lights.contains(id)) {
        m_lights[id] = light;
        m_lights[id].id = id;  // Preserve original ID
        emit lightUpdated(id);
        emit lightsChanged();
        update();
    }
}

PointLight* PointLightSystem::getLight(const QUuid& id)
{
    if (m_lights.contains(id)) {
        return &m_lights[id];
    }
    return nullptr;
}

const PointLight* PointLightSystem::getLight(const QUuid& id) const
{
    auto it = m_lights.constFind(id);
    if (it != m_lights.constEnd()) {
        return &it.value();
    }
    return nullptr;
}

QUuid PointLightSystem::lightAtPosition(const QPointF& pos) const
{
    // Check each light to see if position is within its radius
    for (auto it = m_lights.constBegin(); it != m_lights.constEnd(); ++it) {
        const PointLight& light = it.value();
        qreal dist = QLineF(light.position, pos).length();
        if (dist <= light.radius * 0.2) {  // Hit test on center area
            return it.key();
        }
    }
    return QUuid();
}

void PointLightSystem::toggleLight(const QUuid& id)
{
    if (m_lights.contains(id)) {
        m_lights[id].enabled = !m_lights[id].enabled;
        emit lightUpdated(id);
        update();
    }
}

void PointLightSystem::advanceAnimation(qreal dt)
{
    if (!m_enabled || m_lights.isEmpty()) {
        return;
    }

    // Update flicker phase using driver-provided dt
    m_flickerPhase += dt * 10.0;  // Flicker speed
    if (m_flickerPhase > 1000.0) {
        m_flickerPhase -= 1000.0;
    }

    // Do NOT call update() — SceneAnimationDriver handles scene->update()
}

void PointLightSystem::onFlickerTick()
{
    if (!m_enabled || m_lights.isEmpty()) {
        return;
    }

    // Update flicker phase
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qreal dt = (now - m_lastFlickerTime) / 1000.0;
    m_lastFlickerTime = now;

    m_flickerPhase += dt * 10.0;  // Flicker speed
    if (m_flickerPhase > 1000.0) {
        m_flickerPhase -= 1000.0;
    }

    // Check if any lights are flickering
    bool hasFlickering = false;
    for (const auto& light : m_lights) {
        if (light.flickering && light.enabled) {
            hasFlickering = true;
            break;
        }
    }

    if (hasFlickering) {
        update();
    }
}

void PointLightSystem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                              QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!m_enabled || m_sceneBounds.isEmpty()) {
        return;
    }

    painter->save();

    // First, draw a dark overlay for ambient darkness
    if (m_ambientDarkness > 0.0) {
        int darkAlpha = static_cast<int>(m_ambientDarkness * 180);  // Max 180/255 opacity
        painter->fillRect(m_sceneBounds, QColor(0, 0, 0, darkAlpha));
    }

    // Set composition mode for additive light blending
    painter->setCompositionMode(QPainter::CompositionMode_Plus);

    // Draw each light
    for (const auto& light : m_lights) {
        if (!light.enabled) {
            continue;
        }

        // Calculate flicker modifier
        qreal flickerMod = 1.0;
        if (light.flickering) {
            // Use multiple sine waves for organic flicker
            qreal phase = m_flickerPhase + light.id.toString().length() * 0.1;  // Offset per light
            flickerMod = 1.0 - light.flickerAmount * (
                0.5 * qSin(phase * 2.3) +
                0.3 * qSin(phase * 5.7) +
                0.2 * qSin(phase * 11.1)
            );
            flickerMod = qBound(0.5, flickerMod, 1.2);
        }

        paintLight(painter, light, flickerMod);
    }

    painter->restore();
}

void PointLightSystem::paintLight(QPainter* painter, const PointLight& light, qreal flickerMod)
{
    qreal effectiveIntensity = light.intensity * m_globalIntensity * flickerMod;
    if (effectiveIntensity <= 0.0) {
        return;
    }

    // Create radial gradient for this light
    QRadialGradient gradient(light.position, light.radius);

    // Calculate color with intensity
    QColor centerColor = light.color;
    int alpha = static_cast<int>(qBound(0.0, effectiveIntensity * 255, 255.0));
    centerColor.setAlpha(alpha);

    // Create falloff stops based on falloff curve
    // falloff = 1.0 is linear, 2.0 is quadratic (faster dropoff)
    gradient.setColorAt(0.0, centerColor);

    // Intermediate stops for smoother falloff
    for (qreal t = 0.2; t < 1.0; t += 0.2) {
        qreal falloffFactor = 1.0 - qPow(t, light.falloff);
        int stopAlpha = static_cast<int>(alpha * falloffFactor);
        QColor stopColor = light.color;
        stopColor.setAlpha(qMax(0, stopAlpha));
        gradient.setColorAt(t, stopColor);
    }

    // Fully transparent at edge
    QColor edgeColor = light.color;
    edgeColor.setAlpha(0);
    gradient.setColorAt(1.0, edgeColor);

    // Draw the light circle
    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawEllipse(light.position, light.radius, light.radius);
}
