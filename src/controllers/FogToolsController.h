#ifndef FOGTOOLSCONTROLLER_H
#define FOGTOOLSCONTROLLER_H

#include <QObject>
#include <QAction>
#include <QSlider>
#include <QLabel>
#include "utils/FogToolMode.h"

enum class FogToolMode;
class MapDisplay;

class FogToolsController : public QObject {
    Q_OBJECT
public:
    explicit FogToolsController(QObject* parent = nullptr);

    void setDisplay(MapDisplay* display);

    // Attach existing UI controls; controller will manage their state/labels.
    void attachUI(QAction* unifiedFogAction,
                  QSlider* brushSlider,
                  QLabel* brushLabel,
                  QSlider* gmOpacitySlider,
                  QLabel* gmOpacityLabel);

signals:
    void modeRequested(FogToolMode mode);

public:
    void setMode(FogToolMode mode);
    FogToolMode mode() const { return m_mode; }
    void setBrushSize(int px);
    int brushSize() const { return m_brushSize; }

private:
    void applyMode();

    MapDisplay* m_display {nullptr};
    FogToolMode m_mode;
    int m_brushSize {50};

    // UI pointers (not owned)
    QAction* m_revealRectAction {nullptr};  // Unified fog action (renamed for compatibility)
    QSlider* m_brushSlider {nullptr};
    QLabel* m_brushLabel {nullptr};
    QSlider* m_gmOpacitySlider {nullptr};
    QLabel* m_gmOpacityLabel {nullptr};
};

#endif // FOGTOOLSCONTROLLER_H
