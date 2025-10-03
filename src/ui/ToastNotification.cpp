#include "ui/ToastNotification.h"
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QEvent>
#include <QResizeEvent>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>

ToastNotification::ToastNotification(QWidget* parent)
    : QWidget(parent)
    , m_messageLabel(nullptr)
    , m_hideTimer(nullptr)
    , m_fadeInAnimation(nullptr)
    , m_fadeOutAnimation(nullptr)
    , m_slideAnimation(nullptr)
    , m_shadowEffect(nullptr)
    , m_position(Position::BottomLeft)
    , m_currentType(Type::Info)
    , m_currentOpacity(0.0)
    , m_slidePos(0)
{
    // Widget setup
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedWidth(TOAST_WIDTH);

    // Message label
    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 14px;
            font-weight: 500;
            padding: 16px 20px;
        }
    )");

    // Shadow effect for depth
    m_shadowEffect = new QGraphicsDropShadowEffect(this);
    m_shadowEffect->setBlurRadius(20);
    m_shadowEffect->setXOffset(0);
    m_shadowEffect->setYOffset(4);
    m_shadowEffect->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(m_shadowEffect);

    // Hide timer
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, &ToastNotification::startHideAnimation);

    // Fade in animation
    m_fadeInAnimation = new QPropertyAnimation(this, "opacity");
    m_fadeInAnimation->setDuration(ANIMATION_DURATION);
    m_fadeInAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Fade out animation
    m_fadeOutAnimation = new QPropertyAnimation(this, "opacity");
    m_fadeOutAnimation->setDuration(ANIMATION_DURATION);
    m_fadeOutAnimation->setEasingCurve(QEasingCurve::InCubic);
    connect(m_fadeOutAnimation, &QPropertyAnimation::finished, this, &ToastNotification::onAnimationFinished);

    // Slide animation
    m_slideAnimation = new QPropertyAnimation(this, "slidePosition");
    m_slideAnimation->setDuration(ANIMATION_DURATION);
    m_slideAnimation->setEasingCurve(QEasingCurve::OutBack);

    // Install event filter on parent to track resize
    if (parent) {
        parent->installEventFilter(this);
    }

    hide();
}

ToastNotification::~ToastNotification()
{
    if (m_hideTimer->isActive()) {
        m_hideTimer->stop();
    }
}

void ToastNotification::showMessage(const QString& message, Type type, int duration)
{
    // Stop any running animations
    m_fadeInAnimation->stop();
    m_fadeOutAnimation->stop();
    m_slideAnimation->stop();
    m_hideTimer->stop();

    // Update content
    m_messageLabel->setText(message);
    m_currentType = type;
    setupStyle(type);

    // Adjust height based on content
    QFontMetrics fm(m_messageLabel->font());
    int textHeight = fm.boundingRect(0, 0, TOAST_WIDTH - 40, 0,
                                     Qt::AlignCenter | Qt::TextWordWrap, message).height();
    int totalHeight = qMax(TOAST_MIN_HEIGHT, textHeight + 32); // 32 = padding
    setFixedHeight(totalHeight);
    m_messageLabel->resize(TOAST_WIDTH, totalHeight);

    // Position and show
    updatePosition();
    show();
    raise();

    // Animate in with fade and slide
    m_currentOpacity = 0;
    m_slidePos = -SLIDE_DISTANCE;

    m_fadeInAnimation->setStartValue(0.0);
    m_fadeInAnimation->setEndValue(1.0);

    m_slideAnimation->setStartValue(-SLIDE_DISTANCE);
    m_slideAnimation->setEndValue(0);

    m_fadeInAnimation->start();
    m_slideAnimation->start();

    // Start hide timer
    m_hideTimer->start(duration);
}

void ToastNotification::setOpacity(qreal opacity)
{
    m_currentOpacity = qBound(0.0, opacity, 1.0);

    // Update shadow opacity with main opacity
    if (m_shadowEffect) {
        QColor shadowColor = m_shadowEffect->color();
        shadowColor.setAlphaF(m_currentOpacity * 0.3);
        m_shadowEffect->setColor(shadowColor);
    }

    update();
}

void ToastNotification::setSlidePosition(int pos)
{
    m_slidePos = pos;
    updatePosition();
}

ToastNotification* ToastNotification::instance(QWidget* parent)
{
    static ToastNotification* instance = nullptr;
    if (!instance && parent) {
        instance = new ToastNotification(parent);
    }
    return instance;
}

void ToastNotification::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    if (m_currentOpacity <= 0) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(m_currentOpacity);

    // Draw rounded rectangle background
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);

    // Background color based on type
    QColor bgColor;
    switch (m_currentType) {
        case Type::Success:
            bgColor = QColor(76, 175, 80);  // Green
            break;
        case Type::Warning:
            bgColor = QColor(255, 152, 0);  // Orange
            break;
        case Type::Error:
            bgColor = QColor(244, 67, 54);  // Red
            break;
        case Type::Info:
        default:
            bgColor = QColor(33, 150, 243); // Blue
            break;
    }

    painter.fillPath(path, bgColor);

    // Draw subtle border
    QPen borderPen(bgColor.darker(110), 1);
    painter.setPen(borderPen);
    painter.drawPath(path);
}

bool ToastNotification::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        updatePosition();
    }
    return QWidget::eventFilter(watched, event);
}

void ToastNotification::startHideAnimation()
{
    m_fadeOutAnimation->setStartValue(1.0);
    m_fadeOutAnimation->setEndValue(0.0);
    m_fadeOutAnimation->start();
}

void ToastNotification::onAnimationFinished()
{
    hide();
    m_currentOpacity = 0;
}

void ToastNotification::updatePosition()
{
    if (!parentWidget()) {
        return;
    }

    QWidget* parent = parentWidget();
    int x = 0, y = 0;

    switch (m_position) {
        case Position::TopCenter:
            x = (parent->width() - width()) / 2;
            y = TOAST_MARGIN + m_slidePos;
            break;
        case Position::TopRight:
            x = parent->width() - width() - TOAST_MARGIN;
            y = TOAST_MARGIN + m_slidePos;
            break;
        case Position::BottomCenter:
            x = (parent->width() - width()) / 2;
            y = parent->height() - height() - TOAST_MARGIN - m_slidePos;
            break;
        case Position::BottomRight:
            x = parent->width() - width() - TOAST_MARGIN;
            y = parent->height() - height() - TOAST_MARGIN - m_slidePos;
            break;
        case Position::BottomLeft:
            x = TOAST_MARGIN;
            y = parent->height() - height() - TOAST_MARGIN - m_slidePos;
            break;
    }

    move(x, y);
}

void ToastNotification::setupStyle(Type type)
{
    // Icon would go here if we add icons
    // For now, just ensure the label uses white text (set in constructor)
}