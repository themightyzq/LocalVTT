#include "graphics/FogOfWar.h"
#include "graphics/WallSystem.h"
#include "utils/SecureWindowRegistry.h"
#include <QPainter>
#include <QBrush>
#include <QDataStream>
#include <QBuffer>
#include <QTimer>
#include <QWidget>
#include <QPainterPath>
#include <QPolygonF>
#include <QMutexLocker>

FogOfWar::FogOfWar()
    : m_fogColor(0, 0, 0, 255)  // CRITICAL: Specify full opacity for fog color
    , m_fogOpacity(0.8)
    , m_wallSystem(nullptr)
    , m_gmOpacity(0.3)
    , m_playerViewModeOverride(false)
    , m_ignoreNextChange(false)
    , m_updateTimer(nullptr)
    , m_pendingUpdate(false)
    , m_pixmapCacheValid(false)
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations, false);

    // Initialize update batching timer
    m_updateTimer = new QTimer();
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(16); // ~60 FPS
    QObject::connect(m_updateTimer, &QTimer::timeout, [this]() {
        if (m_pendingUpdate) {
            performDeferredUpdate();
        }
    });
}

FogOfWar::~FogOfWar()
{
    if (m_updateTimer) {
        QObject::disconnect(m_updateTimer, nullptr, nullptr, nullptr);
        m_updateTimer->stop();
        m_updateTimer->deleteLater();
        m_updateTimer = nullptr;
    }
}

void FogOfWar::setMapSize(const QSize& size)
{
    m_mapSize = size;
    initializeFogMask();
    prepareGeometryChange();

    // Clear undo/redo history when loading a new map
    clearHistory();
}

void FogOfWar::initializeFogMask()
{
    if (m_mapSize.isEmpty()) {
        return;
    }

    // Create a mask image with alpha channel
    m_fogMask = QImage(m_mapSize, QImage::Format_ARGB32);
    invalidatePixmapCache();
    fillAll();
}

void FogOfWar::revealArea(const QPointF& center, qreal radius)
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    // Validate bounds to prevent painting outside map area
    QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
    QRectF brushBounds(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

    // Skip if completely outside map bounds
    if (!mapBounds.intersects(brushBounds)) {
        return;
    }

    // Save current state before making changes
    pushState();

    // Direct painting to fog mask for reliability
    QPainter painter(&m_fogMask);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::transparent);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius, radius);

    // Track dirty region for optimized repainting
    QRectF dirtyRect = brushBounds.intersected(mapBounds);
    addDirtyRect(dirtyRect);
    invalidatePixmapCache();

    // Schedule batched update
    scheduleUpdate();
}

void FogOfWar::hideArea(const QPointF& center, qreal radius)
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    // Validate bounds to prevent painting outside map area
    QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
    QRectF brushBounds(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

    // Skip if completely outside map bounds
    if (!mapBounds.intersects(brushBounds)) {
        return;
    }

    // Save current state before making changes
    pushState();

    // Direct painting to fog mask for reliability
    QPainter painter(&m_fogMask);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // CRITICAL: Ensure fog color has full opacity when hiding areas
    QColor opaqueBlack = m_fogColor;
    opaqueBlack.setAlpha(255);
    painter.setBrush(QBrush(opaqueBlack));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius, radius);

    // Track dirty region for optimized repainting
    QRectF dirtyRect = brushBounds.intersected(mapBounds);
    addDirtyRect(dirtyRect);
    invalidatePixmapCache();

    // Schedule batched update
    scheduleUpdate();
}

void FogOfWar::revealRectangle(const QRectF& rect)
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull() || rect.isEmpty()) {
        return;
    }

    // Validate and clamp rectangle to map bounds
    QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
    QRectF clampedRect = rect.intersected(mapBounds);

    // Skip if rectangle is completely outside map bounds
    if (clampedRect.isEmpty()) {
        return;
    }

    // Save current state before making changes
    pushState();

    // Direct painting to fog mask for reliability
    QPainter painter(&m_fogMask);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setBrush(Qt::transparent);
    painter.setPen(Qt::NoPen);
    painter.fillRect(clampedRect, Qt::transparent);

    // Track dirty region for optimized repainting
    addDirtyRect(clampedRect);
    invalidatePixmapCache();

    // Schedule batched update
    scheduleUpdate();
}

void FogOfWar::hideRectangle(const QRectF& rect)
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull() || rect.isEmpty()) {
        return;
    }

    // Validate and clamp rectangle to map bounds
    QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
    QRectF clampedRect = rect.intersected(mapBounds);

    // Skip if rectangle is completely outside map bounds
    if (clampedRect.isEmpty()) {
        return;
    }

    // Save current state before making changes
    pushState();

    QPainter painter(&m_fogMask);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    // CRITICAL: Ensure fog color has full opacity when hiding areas
    QColor opaqueBlack = m_fogColor;
    opaqueBlack.setAlpha(255);
    painter.setBrush(QBrush(opaqueBlack));
    painter.setPen(Qt::NoPen);

    // Use clamped rectangle to prevent painting outside bounds
    painter.fillRect(clampedRect, opaqueBlack);

    // Track dirty region for optimized repainting
    addDirtyRect(clampedRect);

    // Schedule batched update
    scheduleUpdate();
}

void FogOfWar::revealAreaFeathered(const QPointF& center, qreal radius, qreal featherAmount)
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    featherAmount = qBound(0.1, featherAmount, 1.0);

    QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
    QRectF brushBounds(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

    if (!mapBounds.intersects(brushBounds)) {
        return;
    }

    pushState();

    // Direct painting to fog mask for reliability
    QPainter painter(&m_fogMask);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRadialGradient gradient(center, radius);
    gradient.setColorAt(0, QColor(0, 0, 0, 255));
    gradient.setColorAt(1.0 - featherAmount, QColor(0, 0, 0, 255));
    gradient.setColorAt(1.0, QColor(0, 0, 0, 0));

    painter.setBrush(QBrush(gradient));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius, radius);

    // Track dirty region for optimized repainting
    QRectF dirtyRect = brushBounds.intersected(mapBounds);
    addDirtyRect(dirtyRect);
    invalidatePixmapCache();

    scheduleUpdate();
}

void FogOfWar::hideAreaFeathered(const QPointF& center, qreal radius, qreal featherAmount)
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    featherAmount = qBound(0.1, featherAmount, 1.0);

    QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
    QRectF brushBounds(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

    if (!mapBounds.intersects(brushBounds)) {
        return;
    }

    pushState();

    // Direct painting to fog mask for reliability
    QPainter painter(&m_fogMask);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRadialGradient gradient(center, radius);
    QColor fogColor = m_fogColor;
    fogColor.setAlpha(255);
    gradient.setColorAt(0, fogColor);
    gradient.setColorAt(1.0 - featherAmount, fogColor);
    fogColor.setAlpha(0);
    gradient.setColorAt(1.0, fogColor);

    painter.setBrush(QBrush(gradient));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius, radius);

    // Track dirty region for optimized repainting
    QRectF dirtyRect = brushBounds.intersected(mapBounds);
    addDirtyRect(dirtyRect);
    invalidatePixmapCache();

    scheduleUpdate();
}

void FogOfWar::clearAll()
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    // Save current state before making changes
    pushState();

    m_fogMask.fill(Qt::transparent);
    invalidatePixmapCache();

    // Full map update needed
    m_dirtyRegion = boundingRect();
    scheduleUpdate();
}

void FogOfWar::fillAll()
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    // Save current state before making changes
    pushState();

    // CRITICAL FIX: Ensure fog color has full opacity when filling
    // The fog mask uses ARGB32 format, so we need explicit alpha = 255 for opaque black
    QColor opaqueBlack = m_fogColor;
    opaqueBlack.setAlpha(255);  // Force full opacity
    m_fogMask.fill(opaqueBlack.rgba());  // Use rgba() for proper ARGB32 filling
    invalidatePixmapCache();

    // Full map update needed
    m_dirtyRegion = boundingRect();
    scheduleUpdate();
}

void FogOfWar::resetFog()
{
    // Reset fog is equivalent to fillAll() - cover entire map with fog
    fillAll();
}

QRectF FogOfWar::boundingRect() const
{
    return QRectF(0, 0, m_mapSize.width(), m_mapSize.height());
}

void FogOfWar::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)

    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    qreal opacity = 1.0;

    if (widget) {
        SecureWindowRegistry& registry = SecureWindowRegistry::instance();
        SecureWindowRegistry::WindowType windowType = registry.getWindowType(widget);

        if (windowType == SecureWindowRegistry::MainWindow && !m_playerViewModeOverride) {
            opacity = m_gmOpacity;
        }
        else if (windowType == SecureWindowRegistry::PlayerWindow) {
            opacity = 1.0;
        }
    }

    painter->setOpacity(opacity);

    updatePixmapCache();
    if (m_pixmapCacheValid) {
        painter->drawPixmap(0, 0, m_fogPixmapCache);
    } else {
        painter->drawImage(0, 0, m_fogMask);
    }
}

QByteArray FogOfWar::saveState() const
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return QByteArray();
    }

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    
    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_6_0);
    
    // Save fog mask as compressed PNG
    stream << m_mapSize;
    stream << m_fogColor;
    stream << m_fogOpacity;
    
    QByteArray imageData;
    QBuffer imageBuffer(&imageData);
    imageBuffer.open(QIODevice::WriteOnly);
    m_fogMask.save(&imageBuffer, "PNG");
    stream << imageData;
    
    return data;
}

bool FogOfWar::loadState(const QByteArray& data)
{
    QMutexLocker locker(&m_fogMutex);

    if (data.isEmpty()) {
        return false;
    }

    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);
    
    QSize savedMapSize;
    QColor savedFogColor;
    qreal savedFogOpacity;
    QByteArray imageData;
    
    stream >> savedMapSize >> savedFogColor >> savedFogOpacity >> imageData;
    
    // Validate saved data
    if (stream.status() != QDataStream::Ok || savedMapSize.isEmpty()) {
        return false;
    }
    
    // Load fog mask from compressed data
    QImage savedMask;
    if (!savedMask.loadFromData(imageData, "PNG")) {
        return false;
    }
    
    // Apply loaded state
    m_mapSize = savedMapSize;
    m_fogColor = savedFogColor;
    m_fogOpacity = savedFogOpacity;
    m_fogMask = savedMask;
    
    prepareGeometryChange();
    update();
    
    return true;
}

void FogOfWar::pushState()
{
    if (m_fogMask.isNull()) {
        return;
    }

    // Clear redo stack when new action is performed
    m_redoStack.clear();

    // Push current state to undo stack
    QImage copy = m_fogMask.copy();
    m_undoStack.push(copy);
    auto imageBytes = [](const QImage& img){ return static_cast<size_t>(img.width()) * img.height() * 4; };
    m_historyBytes += imageBytes(copy);

    // Limit stack size
    while (m_undoStack.size() > MAX_HISTORY_SIZE || m_historyBytes > MAX_HISTORY_BYTES) {
        if (!m_undoStack.isEmpty()) {
            QImage oldest = m_undoStack.first();
            m_historyBytes -= imageBytes(oldest);
            m_undoStack.removeFirst();
        } else {
            break;
        }
    }
}

void FogOfWar::undo()
{
    QMutexLocker locker(&m_fogMutex);

    if (!canUndo()) {
        return;
    }

    // Push current state to redo stack
    m_redoStack.push(m_fogMask.copy());

    // Restore previous state
    m_ignoreNextChange = true;
    QImage prev = m_undoStack.pop();
    // Adjust history bytes accounting
    m_historyBytes -= static_cast<size_t>(prev.width()) * prev.height() * 4;
    m_fogMask = prev;

    // Force immediate update for undo/redo operations
    forceImmediateUpdate();
    update();
    notifyChange();
    m_ignoreNextChange = false;
}

void FogOfWar::redo()
{
    QMutexLocker locker(&m_fogMutex);

    if (!canRedo()) {
        return;
    }

    // Push current state to undo stack
    QImage copy = m_fogMask.copy();
    m_undoStack.push(copy);
    m_historyBytes += static_cast<size_t>(copy.width()) * copy.height() * 4;

    // Restore next state
    m_ignoreNextChange = true;
    m_fogMask = m_redoStack.pop();

    // Force immediate update for undo/redo operations
    forceImmediateUpdate();
    update();
    notifyChange();
    m_ignoreNextChange = false;
}

void FogOfWar::clearHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();
    m_historyBytes = 0;
}

void FogOfWar::saveCurrentState()
{
    if (!m_fogMask.isNull()) {
        pushState();
    }
}

void FogOfWar::notifyChange()
{
    // CRITICAL FIX: Force immediate update to ensure synchronization
    forceImmediateUpdate();

    // Invalidate pixmap cache to force redraw
    invalidatePixmapCache();

    // Trigger scene update
    update();

    // PRIORITY 4 FIX: Notify listeners about the change with dirty region
    if (m_changeCallback && !m_ignoreNextChange) {
        QRectF dirtyRegion = !m_lastDirtyRegion.isEmpty() ? m_lastDirtyRegion : boundingRect();
        m_changeCallback(dirtyRegion);
    }
}

void FogOfWar::addDirtyRect(const QRectF& rect)
{
    if (m_dirtyRegion.isEmpty()) {
        m_dirtyRegion = rect;
    } else {
        m_dirtyRegion = m_dirtyRegion.united(rect);
    }
}

void FogOfWar::scheduleUpdate()
{
    m_pendingUpdate = true;
    if (!m_updateTimer->isActive()) {
        m_updateTimer->start();
    }
}

void FogOfWar::performDeferredUpdate()
{
    if (!m_pendingUpdate) {
        return;
    }

    m_pendingUpdate = false;

    // PRIORITY 4 FIX: Track last dirty region before clearing
    m_lastDirtyRegion = m_dirtyRegion;

    // Update only the dirty region for better performance
    if (!m_dirtyRegion.isEmpty()) {
        update(m_dirtyRegion);
        m_dirtyRegion = QRectF();
    }

    notifyChange();
}

void FogOfWar::forceImmediateUpdate()
{
    if (m_pendingUpdate) {
        m_updateTimer->stop();
        performDeferredUpdate();
    }
}

void FogOfWar::revealAreaWithWalls(const QPointF& center, qreal radius)
{
    QMutexLocker locker(&m_fogMutex);

    if (m_fogMask.isNull()) {
        return;
    }

    // If no wall system, do simple reveal directly (don't call revealArea to avoid recursion)
    if (!m_wallSystem) {
        // Validate bounds to prevent painting outside map area
        QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
        QRectF brushBounds(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

        // Skip if completely outside map bounds
        if (!mapBounds.intersects(brushBounds)) {
            return;
        }

        // Save current state before making changes
        pushState();

        QPainter painter(&m_fogMask);
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(Qt::transparent);
        painter.setPen(Qt::NoPen);

        // Clip painting to map bounds to prevent corruption
        painter.setClipRect(mapBounds);
        painter.drawEllipse(center, radius, radius);

        // Track dirty region for optimized repainting
        QRectF dirtyRect = brushBounds.intersected(mapBounds);
        addDirtyRect(dirtyRect);

        // Schedule batched update
        scheduleUpdate();
        return;
    }

    // Validate bounds to prevent painting outside map area
    QRectF mapBounds(0, 0, m_mapSize.width(), m_mapSize.height());
    QRectF brushBounds(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

    // Skip if completely outside map bounds
    if (!mapBounds.intersects(brushBounds)) {
        return;
    }

    // Save current state before making changes
    pushState();

    // Get visible area polygon from wall system
    QList<QPointF> visiblePoints = m_wallSystem->getVisibleArea(center, radius);
    
    if (visiblePoints.isEmpty()) {
        // No visibility data, do simple circle reveal directly
        QPainter painter(&m_fogMask);
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(Qt::transparent);
        painter.setPen(Qt::NoPen);
        painter.setClipRect(mapBounds);
        painter.drawEllipse(center, radius, radius);

        QRectF dirtyRect = brushBounds.intersected(mapBounds);
        addDirtyRect(dirtyRect);
        scheduleUpdate();
        return;
    }

    // Create polygon from visible points
    QPolygonF visiblePolygon(visiblePoints);
    
    // Create painter path for the visible area
    QPainterPath visiblePath;
    visiblePath.addPolygon(visiblePolygon);
    
    // Intersect with circle to only reveal within brush radius
    QPainterPath circlePath;
    circlePath.addEllipse(center, radius, radius);
    
    QPainterPath revealPath = visiblePath.intersected(circlePath);

    // Apply the fog reveal using the wall-constrained path
    QPainter painter(&m_fogMask);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::transparent);
    painter.setPen(Qt::NoPen);

    // Clip painting to map bounds to prevent corruption
    painter.setClipRect(mapBounds);
    painter.fillPath(revealPath, Qt::transparent);

    // Track dirty region for optimized repainting
    QRectF dirtyRect = revealPath.boundingRect().intersected(mapBounds);
    addDirtyRect(dirtyRect);

    // Schedule batched update
    scheduleUpdate();
}

void FogOfWar::revealRectangleWithWalls(const QRectF& rect)
{
    // SIMPLIFIED: Just call the regular reveal function
    // Wall system was causing infinite recursion
    revealRectangle(rect);
}

void FogOfWar::invalidatePixmapCache()
{
    m_pixmapCacheValid = false;
}

void FogOfWar::updatePixmapCache()
{
    if (!m_pixmapCacheValid && !m_fogMask.isNull()) {
        m_fogPixmapCache = QPixmap::fromImage(m_fogMask);
        m_pixmapCacheValid = true;
    }
}

void FogOfWar::optimizedPaintOperation(const QRectF& region, std::function<void(QPainter&)> paintFunc)
{
    // SIMPLIFIED: Direct painting is more reliable than region optimization
    // The previous implementation had coordinate translation bugs
    QPainter painter(&m_fogMask);

    // Clip to the region for some optimization
    painter.setClipRect(region);

    // Apply the paint function directly
    paintFunc(painter);

    // Track the dirty region
    addDirtyRect(region);
    invalidatePixmapCache();
}
