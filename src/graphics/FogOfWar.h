#ifndef FOGOFWAR_H
#define FOGOFWAR_H

#include <QGraphicsItem>
#include <QImage>
#include <QSize>
#include <QStack>
#include <QMutex>
#include <QPixmap>
#include <functional>

class QTimer;
class WallSystem;

class FogOfWar : public QGraphicsItem
{
public:
    FogOfWar();
    ~FogOfWar();

    void setMapSize(const QSize& size);
    void setWallSystem(WallSystem* wallSystem) { m_wallSystem = wallSystem; }
    void revealArea(const QPointF& center, qreal radius);
    void hideArea(const QPointF& center, qreal radius);
    void revealRectangle(const QRectF& rect);
    void hideRectangle(const QRectF& rect);
    void revealAreaWithWalls(const QPointF& center, qreal radius);
    void revealRectangleWithWalls(const QRectF& rect);
    void revealAreaFeathered(const QPointF& center, qreal radius, qreal featherAmount = 0.3);
    void hideAreaFeathered(const QPointF& center, qreal radius, qreal featherAmount = 0.3);
    void clearAll();
    void fillAll();
    void resetFog();

    // Serialization methods for autosave
    QByteArray saveState() const;
    bool loadState(const QByteArray& data);
    
    // Get current fog mask for external access
    const QImage& getFogMask() const { return m_fogMask; }
    
    // PRIORITY 4 FIX: Set callback for fog changes with dirty region support
    void setChangeCallback(std::function<void(const QRectF&)> callback) { m_changeCallback = callback; }

    // Undo/Redo system
    void pushState();
    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }
    void undo();
    void redo();
    void clearHistory();

    // DM/Player visual differentiation
    void setGMOpacity(qreal opacity) { m_gmOpacity = qBound(0.0, opacity, 1.0); update(); }
    qreal getGMOpacity() const { return m_gmOpacity; }

    // Player view mode override for DM window
    void setPlayerViewMode(bool enabled) { m_playerViewModeOverride = enabled; update(); }
    bool isPlayerViewMode() const { return m_playerViewModeOverride; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // Force immediate update (for critical operations)
    void forceImmediateUpdate();

private:
    QSize m_mapSize;
    QImage m_fogMask;
    QColor m_fogColor;
    qreal m_fogOpacity;
    WallSystem* m_wallSystem;

    // DM/Player differentiation
    qreal m_gmOpacity;
    bool m_playerViewModeOverride;

    void initializeFogMask();
    void notifyChange();
    std::function<void(const QRectF&)> m_changeCallback;

    // Undo/Redo system
    QStack<QImage> m_undoStack;
    QStack<QImage> m_redoStack;
    static const int MAX_HISTORY_SIZE = 20;
    static const size_t MAX_HISTORY_BYTES = 200 * 1024 * 1024; // 200MB cap
    size_t m_historyBytes = 0;
    bool m_ignoreNextChange;

    void saveCurrentState();

    // Update batching for performance
    QTimer* m_updateTimer;
    bool m_pendingUpdate;
    QRectF m_dirtyRegion;
    void addDirtyRect(const QRectF& rect);
    void scheduleUpdate();
    void performDeferredUpdate();

    // Performance optimizations
    QPixmap m_fogPixmapCache;
    bool m_pixmapCacheValid;
    QRectF m_lastDirtyRegion;
    void invalidatePixmapCache();
    void updatePixmapCache();
    void optimizedPaintOperation(const QRectF& region, std::function<void(QPainter&)> paintFunc);

    // Thread safety
    mutable QMutex m_fogMutex;
};

#endif // FOGOFWAR_H
