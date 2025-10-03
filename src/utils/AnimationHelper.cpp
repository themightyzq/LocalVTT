#include "AnimationHelper.h"
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainter>
#include <QTimer>
#include <QPointer>
#include <cmath>

AnimationHelper::AnimationHelper(QObject* parent)
    : QObject(parent)
{
}

QPropertyAnimation* AnimationHelper::fadeIn(QWidget* widget, int duration,
                                           std::function<void()> onFinished)
{
    if (!widget) return nullptr;

    auto* effect = ensureOpacityEffect(widget);
    effect->setOpacity(0.0);
    widget->setVisible(true);

    auto* anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(duration);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(smoothEasing());

    if (onFinished) {
        QObject::connect(anim, &QPropertyAnimation::finished, onFinished);
    }

    QObject::connect(anim, &QPropertyAnimation::finished, [widget, effect]() {
        if (widget) {
            widget->setGraphicsEffect(nullptr);
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

QPropertyAnimation* AnimationHelper::fadeOut(QWidget* widget, int duration,
                                            std::function<void()> onFinished)
{
    if (!widget) return nullptr;

    auto* effect = ensureOpacityEffect(widget);
    effect->setOpacity(1.0);

    auto* anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(duration);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(smoothEasing());

    QObject::connect(anim, &QPropertyAnimation::finished, [widget, onFinished]() {
        if (widget) {
            widget->setVisible(false);
            widget->setGraphicsEffect(nullptr);
        }
        if (onFinished) {
            onFinished();
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

QPropertyAnimation* AnimationHelper::crossFade(QWidget* oldWidget, QWidget* newWidget,
                                              int duration)
{
    if (oldWidget) {
        fadeOut(oldWidget, duration);
    }
    if (newWidget) {
        return fadeIn(newWidget, duration);
    }
    return nullptr;
}

QPropertyAnimation* AnimationHelper::fadeInGraphicsItem(QGraphicsItem* item, int duration)
{
    if (!item) return nullptr;

    // For QGraphicsObject, we can animate directly
    if (auto* graphicsObject = dynamic_cast<QGraphicsObject*>(item)) {
        graphicsObject->setOpacity(0.0);
        graphicsObject->setVisible(true);

        auto* anim = new QPropertyAnimation(graphicsObject, "opacity");
        anim->setDuration(duration);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(smoothEasing());

        anim->start(QAbstractAnimation::DeleteWhenStopped);
        return anim;
    }

    // For regular QGraphicsItem, just set values directly (no animation)
    item->setOpacity(0.0);
    item->setVisible(true);
    // Gradually increase opacity in steps
    QTimer::singleShot(0, [item]() { if (item) item->setOpacity(0.2); });
    QTimer::singleShot(duration/5, [item]() { if (item) item->setOpacity(0.4); });
    QTimer::singleShot(duration*2/5, [item]() { if (item) item->setOpacity(0.6); });
    QTimer::singleShot(duration*3/5, [item]() { if (item) item->setOpacity(0.8); });
    QTimer::singleShot(duration*4/5, [item]() { if (item) item->setOpacity(1.0); });
    return nullptr;
}

QPropertyAnimation* AnimationHelper::fadeOutGraphicsItem(QGraphicsItem* item, int duration)
{
    if (!item) return nullptr;

    // For QGraphicsObject, we can animate directly
    if (auto* graphicsObject = dynamic_cast<QGraphicsObject*>(item)) {
        auto* anim = new QPropertyAnimation(graphicsObject, "opacity");
        anim->setDuration(duration);
        anim->setStartValue(graphicsObject->opacity());
        anim->setEndValue(0.0);
        anim->setEasingCurve(smoothEasing());

        QObject::connect(anim, &QPropertyAnimation::finished, [graphicsObject]() {
            if (graphicsObject) {
                graphicsObject->setVisible(false);
            }
        });

        anim->start(QAbstractAnimation::DeleteWhenStopped);
        return anim;
    }

    // For regular QGraphicsItem, just fade out in steps
    qreal startOpacity = item->opacity();
    QTimer::singleShot(0, [item, startOpacity]() { if (item) item->setOpacity(startOpacity * 0.8); });
    QTimer::singleShot(duration/5, [item, startOpacity]() { if (item) item->setOpacity(startOpacity * 0.6); });
    QTimer::singleShot(duration*2/5, [item, startOpacity]() { if (item) item->setOpacity(startOpacity * 0.4); });
    QTimer::singleShot(duration*3/5, [item, startOpacity]() { if (item) item->setOpacity(startOpacity * 0.2); });
    QTimer::singleShot(duration*4/5, [item]() { if (item) { item->setOpacity(0.0); item->setVisible(false); } });
    return nullptr;
}

QPropertyAnimation* AnimationHelper::pulseGraphicsItem(QGraphicsItem* item, int duration)
{
    if (!item) return nullptr;

    // For QGraphicsObject, we can animate scale directly
    if (auto* graphicsObject = dynamic_cast<QGraphicsObject*>(item)) {
        auto* anim = new QPropertyAnimation(graphicsObject, "scale");
        anim->setDuration(duration);
        anim->setStartValue(1.0);
        anim->setKeyValueAt(0.5, 1.1);
        anim->setEndValue(1.0);
        anim->setEasingCurve(smoothEasing());

        anim->start(QAbstractAnimation::DeleteWhenStopped);
        return anim;
    }

    // For regular QGraphicsItem, manually pulse with timer
    QTimer::singleShot(0, [item]() { if (item) item->setScale(1.02); });
    QTimer::singleShot(duration/4, [item]() { if (item) item->setScale(1.05); });
    QTimer::singleShot(duration/2, [item]() { if (item) item->setScale(1.1); });
    QTimer::singleShot(duration*3/4, [item]() { if (item) item->setScale(1.05); });
    QTimer::singleShot(duration, [item]() { if (item) item->setScale(1.0); });
    return nullptr;
}

QPropertyAnimation* AnimationHelper::scaleButton(QAbstractButton* button, qreal targetScale,
                                               int duration)
{
    if (!button) return nullptr;

    // Create a subtle scale animation for premium feel
    auto* anim = new QPropertyAnimation(button, "geometry");
    QRect currentGeom = button->geometry();
    QRect targetGeom = currentGeom;

    // Calculate scaled geometry while maintaining center
    int widthDiff = currentGeom.width() * (targetScale - 1.0);
    int heightDiff = currentGeom.height() * (targetScale - 1.0);
    targetGeom.adjust(-widthDiff/2, -heightDiff/2, widthDiff/2, heightDiff/2);

    anim->setDuration(duration);
    anim->setStartValue(currentGeom);
    anim->setEndValue(targetGeom);
    anim->setEasingCurve(standardEasing());

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

QPropertyAnimation* AnimationHelper::bounceButton(QAbstractButton* button)
{
    if (!button) return nullptr;

    auto* anim = scaleButton(button, 1.05, HOVER_DURATION);
    if (anim) {
        anim->setEasingCurve(bounceEasing());
    }
    return anim;
}

void AnimationHelper::showWindowWithFade(QWidget* window, int duration)
{
    if (!window) return;

    window->setWindowOpacity(0.0);
    window->show();

    auto* anim = new QPropertyAnimation(window, "windowOpacity");
    anim->setDuration(duration);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(smoothEasing());

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AnimationHelper::hideWindowWithFade(QWidget* window, int duration)
{
    if (!window) return;

    auto* anim = new QPropertyAnimation(window, "windowOpacity");
    anim->setDuration(duration);
    anim->setStartValue(window->windowOpacity());
    anim->setEndValue(0.0);
    anim->setEasingCurve(smoothEasing());

    QObject::connect(anim, &QPropertyAnimation::finished, [window]() {
        if (window) {
            window->hide();
            window->setWindowOpacity(1.0);
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QPropertyAnimation* AnimationHelper::slideIn(QWidget* widget, Qt::Edge edge, int duration)
{
    if (!widget) return nullptr;

    QRect endGeom = widget->geometry();
    QRect startGeom = endGeom;

    switch (edge) {
        case Qt::LeftEdge:
            startGeom.moveLeft(-endGeom.width());
            break;
        case Qt::RightEdge:
            startGeom.moveLeft(widget->parentWidget() ?
                              widget->parentWidget()->width() : endGeom.width());
            break;
        case Qt::TopEdge:
            startGeom.moveTop(-endGeom.height());
            break;
        case Qt::BottomEdge:
            startGeom.moveTop(widget->parentWidget() ?
                             widget->parentWidget()->height() : endGeom.height());
            break;
    }

    widget->setGeometry(startGeom);
    widget->setVisible(true);

    auto* anim = new QPropertyAnimation(widget, "geometry");
    anim->setDuration(duration);
    anim->setStartValue(startGeom);
    anim->setEndValue(endGeom);
    anim->setEasingCurve(standardEasing());

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

QPropertyAnimation* AnimationHelper::slideOut(QWidget* widget, Qt::Edge edge, int duration)
{
    if (!widget) return nullptr;

    QRect startGeom = widget->geometry();
    QRect endGeom = startGeom;

    switch (edge) {
        case Qt::LeftEdge:
            endGeom.moveLeft(-startGeom.width());
            break;
        case Qt::RightEdge:
            endGeom.moveLeft(widget->parentWidget() ?
                            widget->parentWidget()->width() : startGeom.width());
            break;
        case Qt::TopEdge:
            endGeom.moveTop(-startGeom.height());
            break;
        case Qt::BottomEdge:
            endGeom.moveTop(widget->parentWidget() ?
                           widget->parentWidget()->height() : startGeom.height());
            break;
    }

    auto* anim = new QPropertyAnimation(widget, "geometry");
    anim->setDuration(duration);
    anim->setStartValue(startGeom);
    anim->setEndValue(endGeom);
    anim->setEasingCurve(standardEasing());

    QObject::connect(anim, &QPropertyAnimation::finished, [widget]() {
        if (widget) {
            widget->setVisible(false);
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

QParallelAnimationGroup* AnimationHelper::createToolSwitchAnimation(QWidget* oldTool,
                                                                   QWidget* newTool)
{
    auto* group = new QParallelAnimationGroup();

    if (oldTool) {
        auto* fadeOut = AnimationHelper::fadeOut(oldTool, TOOL_SWITCH_DURATION);
        if (fadeOut) group->addAnimation(fadeOut);
    }

    if (newTool) {
        auto* fadeIn = AnimationHelper::fadeIn(newTool, TOOL_SWITCH_DURATION);
        if (fadeIn) group->addAnimation(fadeIn);
    }

    return group;
}

void AnimationHelper::animateToolHighlight(QWidget* tool, const QColor& highlightColor)
{
    if (!tool) return;

    // Create a temporary highlight effect
    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(highlightColor);
    shadow->setBlurRadius(0);
    shadow->setOffset(0, 0);
    tool->setGraphicsEffect(shadow);

    auto* anim = new QPropertyAnimation(shadow, "blurRadius");
    anim->setDuration(HOVER_DURATION);
    anim->setStartValue(0);
    anim->setEndValue(15);
    anim->setEasingCurve(standardEasing());

    QObject::connect(anim, &QPropertyAnimation::finished, [tool]() {
        // Fade out the highlight
        auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(tool->graphicsEffect());
        if (shadow) {
            auto* fadeAnim = new QPropertyAnimation(shadow, "blurRadius");
            fadeAnim->setDuration(HOVER_DURATION);
            fadeAnim->setStartValue(15);
            fadeAnim->setEndValue(0);
            fadeAnim->setEasingCurve(standardEasing());

            QObject::connect(fadeAnim, &QPropertyAnimation::finished, [tool]() {
                tool->setGraphicsEffect(nullptr);
            });

            fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QPropertyAnimation* AnimationHelper::animateBrushPreview(QGraphicsItem* preview, bool show,
                                                        int duration)
{
    if (!preview) return nullptr;

    // Use our existing fade methods
    if (show) {
        preview->setVisible(true);
        return fadeInGraphicsItem(preview, duration);
    } else {
        return fadeOutGraphicsItem(preview, duration);
    }
}

void AnimationHelper::pulseBrushPreview(QGraphicsItem* preview)
{
    if (!preview) return;

    // For QGraphicsObject, we can create a pulsing opacity animation
    if (auto* graphicsObject = dynamic_cast<QGraphicsObject*>(preview)) {
        auto* anim = new QPropertyAnimation(graphicsObject, "opacity");
        anim->setDuration(1000);
        anim->setStartValue(1.0);
        anim->setKeyValueAt(0.5, 0.5);
        anim->setEndValue(1.0);
        anim->setEasingCurve(smoothEasing());
        anim->setLoopCount(-1); // Infinite loop

        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        // For regular QGraphicsItem, create a repeating timer-based pulse
        class PulseHelper : public QObject {
        public:
            PulseHelper(QGraphicsItem* item) : m_item(item), m_phase(0) {
                m_timer = new QTimer(this);
                m_timer->setInterval(50); // 20 FPS
                connect(m_timer, &QTimer::timeout, this, &PulseHelper::updatePulse);
                m_timer->start();
            }
        private:
            void updatePulse() {
                if (!m_item) {
                    deleteLater();
                    return;
                }
                m_phase += 0.1;
                qreal opacity = 0.75 + 0.25 * sin(m_phase);
                m_item->setOpacity(opacity);
            }
            QGraphicsItem* m_item;
            QTimer* m_timer;
            qreal m_phase;
        };
        new PulseHelper(preview); // Will auto-delete when item is destroyed
    }
}

QPropertyAnimation* AnimationHelper::animateStatusChange(QWidget* indicator,
                                                        const QColor& newColor,
                                                        int duration)
{
    if (!indicator) return nullptr;

    QString oldStyle = indicator->styleSheet();
    QString newStyle = QString("background-color: %1; border-radius: 4px; padding: 4px;")
                              .arg(newColor.name());

    // Animate using opacity transition
    auto* effect = ensureOpacityEffect(indicator);

    auto* fadeOut = new QPropertyAnimation(effect, "opacity");
    fadeOut->setDuration(duration / 2);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(standardEasing());

    QObject::connect(fadeOut, &QPropertyAnimation::finished, [indicator, newStyle, effect, duration]() {
        indicator->setStyleSheet(newStyle);

        auto* fadeIn = new QPropertyAnimation(effect, "opacity");
        fadeIn->setDuration(duration / 2);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->setEasingCurve(standardEasing());

        QObject::connect(fadeIn, &QPropertyAnimation::finished, [indicator]() {
            indicator->setGraphicsEffect(nullptr);
        });

        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    return fadeOut;
}

void AnimationHelper::flashStatusIndicator(QWidget* indicator, const QColor& flashColor)
{
    if (!indicator) return;

    QString originalStyle = indicator->styleSheet();
    QString flashStyle = QString("background-color: %1; border-radius: 4px; padding: 4px;")
                                .arg(flashColor.name());

    indicator->setStyleSheet(flashStyle);

    QTimer::singleShot(100, [indicator, originalStyle]() {
        if (indicator) {
            animateStatusChange(indicator, QColor(originalStyle.mid(17, 7)), HOVER_DURATION);
        }
    });
}

void AnimationHelper::addHoverGlow(QWidget* widget, const QColor& glowColor)
{
    if (!widget) return;

    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(glowColor);
    shadow->setBlurRadius(0);
    shadow->setOffset(0, 0);
    widget->setGraphicsEffect(shadow);

    auto* anim = new QPropertyAnimation(shadow, "blurRadius");
    anim->setDuration(HOVER_DURATION);
    anim->setStartValue(0);
    anim->setEndValue(20);
    anim->setEasingCurve(standardEasing());

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AnimationHelper::removeHoverGlow(QWidget* widget)
{
    if (!widget) return;

    auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(widget->graphicsEffect());
    if (!shadow) return;

    auto* anim = new QPropertyAnimation(shadow, "blurRadius");
    anim->setDuration(HOVER_DURATION);
    anim->setStartValue(shadow->blurRadius());
    anim->setEndValue(0);
    anim->setEasingCurve(standardEasing());

    QObject::connect(anim, &QPropertyAnimation::finished, [widget]() {
        widget->setGraphicsEffect(nullptr);
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QPropertyAnimation* AnimationHelper::animateShadow(QWidget* widget, int startBlur, int endBlur,
                                                  int duration)
{
    if (!widget) return nullptr;

    auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(widget->graphicsEffect());
    if (!shadow) {
        shadow = new QGraphicsDropShadowEffect();
        shadow->setColor(QColor(0, 0, 0, 100));
        shadow->setOffset(0, 2);
        widget->setGraphicsEffect(shadow);
    }

    auto* anim = new QPropertyAnimation(shadow, "blurRadius");
    anim->setDuration(duration);
    anim->setStartValue(startBlur);
    anim->setEndValue(endBlur);
    anim->setEasingCurve(standardEasing());

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

void AnimationHelper::smoothScroll(QAbstractScrollArea* scrollArea, const QPoint& targetPos,
                                  int duration)
{
    if (!scrollArea) return;

    QScrollBar* hBar = scrollArea->horizontalScrollBar();
    QScrollBar* vBar = scrollArea->verticalScrollBar();

    if (hBar && hBar->isVisible()) {
        auto* hAnim = new QPropertyAnimation(hBar, "value");
        hAnim->setDuration(duration);
        hAnim->setStartValue(hBar->value());
        hAnim->setEndValue(targetPos.x());
        hAnim->setEasingCurve(smoothEasing());
        hAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    if (vBar && vBar->isVisible()) {
        auto* vAnim = new QPropertyAnimation(vBar, "value");
        vAnim->setDuration(duration);
        vAnim->setStartValue(vBar->value());
        vAnim->setEndValue(targetPos.y());
        vAnim->setEasingCurve(smoothEasing());
        vAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void AnimationHelper::setupButtonHoverAnimation(QAbstractButton* button,
                                               const QColor& hoverColor)
{
    if (!button) return;

    // Store original style
    QString originalStyle = button->styleSheet();

    // Connect hover events
    button->installEventFilter(new QObject());

    QObject::connect(button, &QAbstractButton::pressed, [button]() {
        scaleButton(button, 0.95, 100);
    });

    QObject::connect(button, &QAbstractButton::released, [button]() {
        scaleButton(button, 1.0, 100);
    });
}

void AnimationHelper::setupToolButtonAnimation(QToolButton* button)
{
    if (!button) return;

    // Add hover effects specific to tool buttons
    setupButtonHoverAnimation(button, QColor(74, 144, 226, 80));

    // Add checked state animation
    QObject::connect(button, &QToolButton::toggled, [button](bool checked) {
        if (checked) {
            addHoverGlow(button, QColor(74, 144, 226));
        } else {
            removeHoverGlow(button);
        }
    });
}

QPropertyAnimation* AnimationHelper::createLoadingAnimation(QWidget* loadingWidget)
{
    if (!loadingWidget) return nullptr;

    auto* anim = new QPropertyAnimation(loadingWidget, "rotation");
    anim->setDuration(1000);
    anim->setStartValue(0);
    anim->setEndValue(360);
    anim->setEasingCurve(QEasingCurve::Linear);
    anim->setLoopCount(-1);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

void AnimationHelper::startPulseAnimation(QWidget* widget)
{
    if (!widget) return;

    auto* effect = ensureOpacityEffect(widget);

    auto* anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(1500);
    anim->setStartValue(1.0);
    anim->setKeyValueAt(0.5, 0.3);
    anim->setEndValue(1.0);
    anim->setEasingCurve(smoothEasing());
    anim->setLoopCount(-1);

    // Store animation in widget property for later cleanup
    widget->setProperty("pulseAnimation", QVariant::fromValue(anim));

    anim->start();
}

void AnimationHelper::stopPulseAnimation(QWidget* widget)
{
    if (!widget) return;

    QVariant animVar = widget->property("pulseAnimation");
    if (animVar.isValid()) {
        auto* anim = animVar.value<QPropertyAnimation*>();
        if (anim) {
            anim->stop();
            anim->deleteLater();
        }
    }

    widget->setProperty("pulseAnimation", QVariant());
    widget->setGraphicsEffect(nullptr);
}

QParallelAnimationGroup* AnimationHelper::batchFadeIn(const QList<QWidget*>& widgets,
                                                     int staggerDelay)
{
    auto* group = new QParallelAnimationGroup();

    int delay = 0;
    for (QWidget* widget : widgets) {
        if (widget) {
            QTimer::singleShot(delay, [widget]() {
                fadeIn(widget, FADE_DURATION);
            });
            delay += staggerDelay;
        }
    }

    return group;
}

QParallelAnimationGroup* AnimationHelper::batchFadeOut(const QList<QWidget*>& widgets,
                                                      int staggerDelay)
{
    auto* group = new QParallelAnimationGroup();

    int delay = 0;
    for (QWidget* widget : widgets) {
        if (widget) {
            QTimer::singleShot(delay, [widget]() {
                fadeOut(widget, FADE_DURATION);
            });
            delay += staggerDelay;
        }
    }

    return group;
}

QGraphicsOpacityEffect* AnimationHelper::ensureOpacityEffect(QWidget* widget)
{
    if (!widget) return nullptr;

    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect();
        widget->setGraphicsEffect(effect);
    }
    return effect;
}

void AnimationHelper::cleanupEffect(QWidget* widget, QGraphicsEffect* effect)
{
    if (widget && widget->graphicsEffect() == effect) {
        widget->setGraphicsEffect(nullptr);
    }
}