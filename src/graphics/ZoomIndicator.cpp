#include "graphics/ZoomIndicator.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QPainterPath>

ZoomIndicator::ZoomIndicator(QWidget *parent)
    : QWidget(parent)
    , m_zoomFactor(1.0)
    , m_opacity(0.0)
    , m_hideTimer(nullptr)
    , m_fadeAnimation(nullptr)
{
    // Set widget properties
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    
    // Initialize hide timer
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, &ZoomIndicator::hideIndicator);
    
    // Initialize fade animation
    m_fadeAnimation = new QPropertyAnimation(this, "opacity");
    m_fadeAnimation->setDuration(FADE_DURATION);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    
    // Set initial size and position
    resize(INDICATOR_WIDTH, INDICATOR_HEIGHT);
    updatePosition();
    
    // Start hidden
    hide();
}

ZoomIndicator::~ZoomIndicator()
{
    if (m_fadeAnimation && m_fadeAnimation->state() == QAbstractAnimation::Running) {
        m_fadeAnimation->stop();
    }
    if (m_hideTimer && m_hideTimer->isActive()) {
        m_hideTimer->stop();
    }
}

void ZoomIndicator::showZoom(qreal zoomFactor)
{
    m_zoomFactor = zoomFactor;
    
    // Stop any running animations/timers
    if (m_fadeAnimation->state() == QAbstractAnimation::Running) {
        m_fadeAnimation->stop();
    }
    if (m_hideTimer->isActive()) {
        m_hideTimer->stop();
    }
    
    // Update position in case parent was resized
    updatePosition();
    
    // Fade in immediately
    m_fadeAnimation->setStartValue(m_opacity);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
    
    // Show the widget
    show();
    raise();
    
    // Start timer to hide after display duration
    m_hideTimer->start(DISPLAY_DURATION);
}

void ZoomIndicator::setOpacity(qreal opacity)
{
    m_opacity = qBound(0.0, opacity, 1.0);
    update();
}

void ZoomIndicator::updatePosition()
{
    if (!parentWidget()) {
        return;
    }
    
    // Position in top-right corner of parent
    QWidget* parent = parentWidget();
    int x = parent->width() - width() - CORNER_MARGIN;
    int y = CORNER_MARGIN;
    
    move(x, y);
}

void ZoomIndicator::hideIndicator()
{
    // Fade out
    if (m_fadeAnimation->state() == QAbstractAnimation::Running) {
        m_fadeAnimation->stop();
    }
    
    m_fadeAnimation->setStartValue(m_opacity);
    m_fadeAnimation->setEndValue(0.0);
    
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, nullptr);
    });
    
    m_fadeAnimation->start();
}

void ZoomIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    if (m_opacity <= 0.0) {
        return;
    }
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Calculate zoom percentage
    int percentage = static_cast<int>(m_zoomFactor * 100 + 0.5);
    QString zoomText = QString("%1%").arg(percentage);
    
    // Set up font
    QFont font("Arial", 12, QFont::Bold);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(zoomText);
    
    // Center text in widget
    QRect drawRect = rect();
    textRect.moveCenter(drawRect.center());
    
    // Draw background with rounded corners
    QPainterPath backgroundPath;
    backgroundPath.addRoundedRect(drawRect, 5, 5);
    
    // Semi-transparent dark background
    QColor backgroundColor(20, 20, 20, static_cast<int>(200 * m_opacity));
    painter.fillPath(backgroundPath, backgroundColor);
    
    // Draw border
    QPen borderPen(QColor(100, 200, 255, static_cast<int>(150 * m_opacity)), 2);
    painter.setPen(borderPen);
    painter.drawPath(backgroundPath);
    
    // Draw text
    QColor textColor(255, 255, 255, static_cast<int>(255 * m_opacity));
    painter.setPen(textColor);
    painter.drawText(textRect, Qt::AlignCenter, zoomText);
}

void ZoomIndicator::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePosition();
}