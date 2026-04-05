#include "graphics/SceneAnimationDriver.h"
#include <QGraphicsScene>

SceneAnimationDriver::SceneAnimationDriver(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_boostTimer(new QTimer(this))
{
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(1000 / m_targetFPS);
    connect(m_timer, &QTimer::timeout, this, &SceneAnimationDriver::onTimeout);

    m_boostTimer->setSingleShot(true);
    connect(m_boostTimer, &QTimer::timeout, this, &SceneAnimationDriver::onBoostExpired);

    m_elapsed.start();
}

void SceneAnimationDriver::start()
{
    m_elapsed.restart();
    m_timer->start();
}

void SceneAnimationDriver::stop()
{
    m_timer->stop();
}

void SceneAnimationDriver::setTargetFPS(int fps)
{
    m_targetFPS = qBound(1, fps, 120);
    m_baseFPS = m_targetFPS;
    m_timer->setInterval(1000 / m_targetFPS);
}

void SceneAnimationDriver::boostFPS(int fps, int durationMs)
{
    int boosted = qBound(1, fps, 120);
    m_timer->setInterval(1000 / boosted);
    m_boostTimer->start(durationMs);
}

void SceneAnimationDriver::onBoostExpired()
{
    m_timer->setInterval(1000 / m_baseFPS);
}

void SceneAnimationDriver::onTimeout()
{
    qreal dt = m_elapsed.restart() / 1000.0;
    dt = qMin(dt, 0.1);  // Cap to prevent huge jumps after stalls

    emit tick(dt);

    // Single scene update for all animated items
    if (m_scene) {
        m_scene->update();
    }
}
