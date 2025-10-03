#ifndef ANIMATIONHELPER_H
#define ANIMATIONHELPER_H

#include <QObject>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QWidget>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QPushButton>
#include <QToolButton>
#include <QAbstractScrollArea>
#include <functional>

class AnimationHelper : public QObject
{
    Q_OBJECT

public:
    // Animation duration constants (following CLAUDE.md standards)
    static constexpr int STANDARD_DURATION = 150;     // Standard transition: 150ms
    static constexpr int SMOOTH_DURATION = 200;       // Smooth transition: 200ms
    static constexpr int FADE_DURATION = 200;         // Fade effects: 200ms
    static constexpr int TOOL_SWITCH_DURATION = 150;  // Tool switching: 150ms
    static constexpr int HOVER_DURATION = 150;        // Hover states: 150ms
    static constexpr int TOAST_DURATION = 2000;       // Toast auto-dismiss: 2000ms

    // Standard easing curves for premium feel
    static QEasingCurve standardEasing() { return QEasingCurve::OutCubic; }
    static QEasingCurve smoothEasing() { return QEasingCurve::InOutCubic; }
    static QEasingCurve bounceEasing() { return QEasingCurve::OutBack; }
    static QEasingCurve elasticEasing() { return QEasingCurve::OutElastic; }

    explicit AnimationHelper(QObject* parent = nullptr);

    // Widget fade animations
    static QPropertyAnimation* fadeIn(QWidget* widget, int duration = FADE_DURATION,
                                     std::function<void()> onFinished = nullptr);
    static QPropertyAnimation* fadeOut(QWidget* widget, int duration = FADE_DURATION,
                                      std::function<void()> onFinished = nullptr);
    static QPropertyAnimation* crossFade(QWidget* oldWidget, QWidget* newWidget,
                                        int duration = FADE_DURATION);

    // Graphics item animations
    static QPropertyAnimation* fadeInGraphicsItem(QGraphicsItem* item, int duration = FADE_DURATION);
    static QPropertyAnimation* fadeOutGraphicsItem(QGraphicsItem* item, int duration = FADE_DURATION);
    static QPropertyAnimation* pulseGraphicsItem(QGraphicsItem* item, int duration = SMOOTH_DURATION);

    // Scale animations for buttons and tools
    static QPropertyAnimation* scaleButton(QAbstractButton* button, qreal targetScale,
                                          int duration = HOVER_DURATION);
    static QPropertyAnimation* bounceButton(QAbstractButton* button);

    // Window and dialog animations
    static void showWindowWithFade(QWidget* window, int duration = FADE_DURATION);
    static void hideWindowWithFade(QWidget* window, int duration = FADE_DURATION);
    static QPropertyAnimation* slideIn(QWidget* widget, Qt::Edge edge, int duration = SMOOTH_DURATION);
    static QPropertyAnimation* slideOut(QWidget* widget, Qt::Edge edge, int duration = SMOOTH_DURATION);

    // Tool transition effects
    static QParallelAnimationGroup* createToolSwitchAnimation(QWidget* oldTool, QWidget* newTool);
    static void animateToolHighlight(QWidget* tool, const QColor& highlightColor);

    // Brush preview animations
    static QPropertyAnimation* animateBrushPreview(QGraphicsItem* preview, bool show,
                                                  int duration = STANDARD_DURATION);
    static void pulseBrushPreview(QGraphicsItem* preview);

    // Status indicator animations
    static QPropertyAnimation* animateStatusChange(QWidget* indicator, const QColor& newColor,
                                                  int duration = STANDARD_DURATION);
    static void flashStatusIndicator(QWidget* indicator, const QColor& flashColor);

    // Shadow and glow effects
    static void addHoverGlow(QWidget* widget, const QColor& glowColor = QColor(74, 144, 226));
    static void removeHoverGlow(QWidget* widget);
    static QPropertyAnimation* animateShadow(QWidget* widget, int startBlur, int endBlur,
                                            int duration = HOVER_DURATION);

    // Smooth scrolling
    static void smoothScroll(QAbstractScrollArea* scrollArea, const QPoint& targetPos,
                            int duration = SMOOTH_DURATION);

    // Button hover animations
    static void setupButtonHoverAnimation(QAbstractButton* button,
                                         const QColor& hoverColor = QColor(74, 144, 226, 50));
    static void setupToolButtonAnimation(QToolButton* button);

    // Loading and progress animations
    static QPropertyAnimation* createLoadingAnimation(QWidget* loadingWidget);
    static void startPulseAnimation(QWidget* widget);
    static void stopPulseAnimation(QWidget* widget);

    // Helper to ensure animation cleanup
    template<typename T>
    static void safeAnimate(T* target, const QByteArray& property, const QVariant& endValue,
                           int duration = STANDARD_DURATION,
                           std::function<void()> onFinished = nullptr) {
        auto* anim = new QPropertyAnimation(target, property);
        anim->setDuration(duration);
        anim->setEndValue(endValue);
        anim->setEasingCurve(standardEasing());

        if (onFinished) {
            QObject::connect(anim, &QPropertyAnimation::finished, onFinished);
        }

        QObject::connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
        anim->start();
    }

    // Batch animation for multiple widgets
    static QParallelAnimationGroup* batchFadeIn(const QList<QWidget*>& widgets,
                                               int staggerDelay = 50);
    static QParallelAnimationGroup* batchFadeOut(const QList<QWidget*>& widgets,
                                                int staggerDelay = 50);

private:
    // Internal helper methods
    static QGraphicsOpacityEffect* ensureOpacityEffect(QWidget* widget);
    static void cleanupEffect(QWidget* widget, QGraphicsEffect* effect);
};

#endif // ANIMATIONHELPER_H