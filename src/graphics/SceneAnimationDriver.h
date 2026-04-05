#ifndef SCENEANIMATIONDRIVER_H
#define SCENEANIMATIONDRIVER_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

class QGraphicsScene;

class SceneAnimationDriver : public QObject {
    Q_OBJECT
public:
    explicit SceneAnimationDriver(QObject* parent = nullptr);

    void setScene(QGraphicsScene* scene) { m_scene = scene; }

    void start();
    void stop();
    bool isRunning() const { return m_timer->isActive(); }

    void setTargetFPS(int fps);
    int targetFPS() const { return m_targetFPS; }

    // Temporarily boost FPS for transitions, auto-reverts after duration
    void boostFPS(int fps, int durationMs);

signals:
    void tick(qreal deltaTime);

private slots:
    void onTimeout();
    void onBoostExpired();

private:
    QTimer* m_timer;
    QElapsedTimer m_elapsed;
    QGraphicsScene* m_scene = nullptr;
    int m_targetFPS = 30;
    int m_baseFPS = 30;
    QTimer* m_boostTimer;
};

#endif // SCENEANIMATIONDRIVER_H
