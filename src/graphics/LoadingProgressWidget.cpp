#include "graphics/LoadingProgressWidget.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QPainterPath>
#include <QLinearGradient>

LoadingProgressWidget::LoadingProgressWidget(QWidget *parent)
    : QWidget(parent)
    , m_progress(0)
    , m_loadingText("Loading...")
    , m_opacity(0.0)
    , m_fadeAnimation(nullptr)
{
    // Set widget properties
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);

    // Initialize fade animation
    m_fadeAnimation = new QPropertyAnimation(this, "opacity");
    m_fadeAnimation->setDuration(FADE_DURATION);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // Set initial size and position
    resize(WIDGET_WIDTH, WIDGET_HEIGHT);
    updatePosition();

    // Start hidden
    hide();
}

LoadingProgressWidget::~LoadingProgressWidget()
{
    if (m_fadeAnimation && m_fadeAnimation->state() == QAbstractAnimation::Running) {
        m_fadeAnimation->stop();
    }
}

void LoadingProgressWidget::showProgress()
{
    // Stop any running animations
    if (m_fadeAnimation->state() == QAbstractAnimation::Running) {
        m_fadeAnimation->stop();
    }

    // Update position in case parent was resized
    updatePosition();

    // Fade in
    m_fadeAnimation->setStartValue(m_opacity);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();

    // Show the widget
    show();
    raise();
}

void LoadingProgressWidget::hideProgress()
{
    // Stop any running animations
    if (m_fadeAnimation->state() == QAbstractAnimation::Running) {
        m_fadeAnimation->stop();
    }

    // Fade out
    m_fadeAnimation->setStartValue(m_opacity);
    m_fadeAnimation->setEndValue(0.0);

    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, nullptr);
    });

    m_fadeAnimation->start();
}

void LoadingProgressWidget::setProgress(int percentage)
{
    m_progress = qBound(0, percentage, 100);
    update();
}

void LoadingProgressWidget::setLoadingText(const QString& text)
{
    m_loadingText = text;
    update();
}

void LoadingProgressWidget::setOpacity(qreal opacity)
{
    m_opacity = qBound(0.0, opacity, 1.0);
    update();
}

void LoadingProgressWidget::updatePosition()
{
    if (!parentWidget()) {
        return;
    }

    // Position in center of parent
    QWidget* parent = parentWidget();
    int x = (parent->width() - width()) / 2;
    int y = (parent->height() - height()) / 2;

    move(x, y);
}

void LoadingProgressWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (m_opacity <= 0.0) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect widgetRect = rect();

    // Draw background with rounded corners
    QPainterPath backgroundPath;
    backgroundPath.addRoundedRect(widgetRect, 8, 8);

    // Semi-transparent dark background with glass effect
    QColor backgroundColor(20, 20, 20, static_cast<int>(220 * m_opacity));
    painter.fillPath(backgroundPath, backgroundColor);

    // Draw border
    QPen borderPen(QColor(100, 200, 255, static_cast<int>(180 * m_opacity)), 2);
    painter.setPen(borderPen);
    painter.drawPath(backgroundPath);

    // Setup font for text
    QFont font("Arial", 11, QFont::Normal);
    painter.setFont(font);

    QFontMetrics fm(font);

    // Calculate layout
    int textHeight = fm.height();
    int padding = 12;
    int progressBarY = padding + textHeight + 8;

    // Draw loading text
    QColor textColor(255, 255, 255, static_cast<int>(255 * m_opacity));
    painter.setPen(textColor);

    QRect textRect(padding, padding, widgetRect.width() - 2 * padding, textHeight);
    painter.drawText(textRect, Qt::AlignCenter, m_loadingText);

    // Draw progress bar background
    QRect progressBgRect(padding, progressBarY,
                        widgetRect.width() - 2 * padding, PROGRESS_BAR_HEIGHT);

    QPainterPath progressBgPath;
    progressBgPath.addRoundedRect(progressBgRect, PROGRESS_BAR_HEIGHT / 2, PROGRESS_BAR_HEIGHT / 2);

    QColor progressBgColor(60, 60, 60, static_cast<int>(200 * m_opacity));
    painter.fillPath(progressBgPath, progressBgColor);

    // Draw progress bar fill
    if (m_progress > 0) {
        int fillWidth = (progressBgRect.width() * m_progress) / 100;
        QRect progressFillRect(progressBgRect.x(), progressBgRect.y(),
                              fillWidth, progressBgRect.height());

        QPainterPath progressFillPath;
        progressFillPath.addRoundedRect(progressFillRect, PROGRESS_BAR_HEIGHT / 2, PROGRESS_BAR_HEIGHT / 2);

        // Create gradient for progress bar
        QLinearGradient gradient(progressFillRect.topLeft(), progressFillRect.bottomLeft());
        gradient.setColorAt(0, QColor(100, 200, 255, static_cast<int>(255 * m_opacity)));
        gradient.setColorAt(1, QColor(50, 150, 255, static_cast<int>(255 * m_opacity)));

        painter.fillPath(progressFillPath, gradient);
    }

    // Draw percentage text
    QString percentText = QString("%1%").arg(m_progress);
    QRect percentRect(padding, progressBarY + PROGRESS_BAR_HEIGHT + 4,
                     widgetRect.width() - 2 * padding, textHeight);

    QFont percentFont("Arial", 9, QFont::Normal);
    painter.setFont(percentFont);

    QColor percentColor(200, 200, 200, static_cast<int>(255 * m_opacity));
    painter.setPen(percentColor);
    painter.drawText(percentRect, Qt::AlignCenter, percentText);
}

void LoadingProgressWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePosition();
}