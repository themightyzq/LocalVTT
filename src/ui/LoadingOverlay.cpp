#include "ui/LoadingOverlay.h"
#include <QLabel>
#include <QProgressBar>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

LoadingOverlay::LoadingOverlay(QWidget* parent)
    : QWidget(parent)
    , m_messageLabel(nullptr)
    , m_subMessageLabel(nullptr)
    , m_elapsedTimeLabel(nullptr)
    , m_progressBar(nullptr)
    , m_contentWidget(nullptr)
    , m_fadeAnimation(nullptr)
    , m_pulseAnimation(nullptr)
    , m_animationGroup(nullptr)
    , m_opacityEffect(nullptr)
    , m_elapsedTimer(nullptr)
    , m_isLoading(false)
    , m_isIndeterminate(true)
    , m_currentOpacity(0.0)
    , m_pulseGlowValue(0)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    setupUI();
    applyProfessionalStyling();

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    m_fadeAnimation = new QPropertyAnimation(this, "opacity");
    m_fadeAnimation->setDuration(FADE_DURATION);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_fadeAnimation, &QPropertyAnimation::finished,
            this, [this]() {
                if (m_currentOpacity <= 0) {
                    onFadeOutFinished();
                } else {
                    onFadeInFinished();
                }
            });

    m_pulseAnimation = new QPropertyAnimation(this, "pulseGlow");
    m_pulseAnimation->setDuration(PULSE_DURATION);
    m_pulseAnimation->setEasingCurve(QEasingCurve::InOutSine);
    m_pulseAnimation->setLoopCount(-1);
    m_pulseAnimation->setStartValue(0);
    m_pulseAnimation->setEndValue(100);

    m_elapsedTimer = new QTimer(this);
    m_elapsedTimer->setInterval(100);
    connect(m_elapsedTimer, &QTimer::timeout,
            this, &LoadingOverlay::updateElapsedTime);

    if (parent) {
        parent->installEventFilter(this);
    }

    hide();
}

LoadingOverlay::~LoadingOverlay()
{
    stopAnimations();
}

void LoadingOverlay::setupUI()
{
    m_contentWidget = new QWidget(this);
    m_contentWidget->setFixedSize(CONTENT_WIDTH, CONTENT_HEIGHT);

    auto* layout = new QVBoxLayout(m_contentWidget);
    layout->setSpacing(12);
    layout->setContentsMargins(30, 30, 30, 30);

    m_messageLabel = new QLabel(m_contentWidget);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 16px;
            font-weight: 600;
        }
    )");
    layout->addWidget(m_messageLabel);

    m_progressBar = new QProgressBar(m_contentWidget);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    layout->addWidget(m_progressBar);

    m_subMessageLabel = new QLabel(m_contentWidget);
    m_subMessageLabel->setAlignment(Qt::AlignCenter);
    m_subMessageLabel->setStyleSheet(R"(
        QLabel {
            color: rgba(255, 255, 255, 0.7);
            font-size: 13px;
        }
    )");
    layout->addWidget(m_subMessageLabel);

    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(0);

    m_elapsedTimeLabel = new QLabel(m_contentWidget);
    m_elapsedTimeLabel->setAlignment(Qt::AlignCenter);
    m_elapsedTimeLabel->setStyleSheet(R"(
        QLabel {
            color: rgba(255, 255, 255, 0.5);
            font-size: 11px;
        }
    )");
    bottomLayout->addWidget(m_elapsedTimeLabel);

    layout->addLayout(bottomLayout);
    layout->addStretch();
}

void LoadingOverlay::showLoading(const QString& message,
                                bool indeterminate,
                                int progressMax)
{
    if (m_isLoading) {
        return;
    }

    m_isLoading = true;
    m_isIndeterminate = indeterminate;
    m_currentMessage = message.isEmpty() ? "Loading..." : message;

    m_messageLabel->setText(m_currentMessage);
    m_subMessageLabel->clear();
    m_elapsedTimeLabel->clear();

    if (indeterminate) {
        m_progressBar->setRange(0, 0);
    } else {
        m_progressBar->setRange(0, progressMax);
        m_progressBar->setValue(0);
    }

    centerOnParent();
    show();
    raise();

    m_loadTimer.start();
    m_elapsedTimer->start();

    startAnimations();
}

void LoadingOverlay::updateProgress(int value, const QString& subMessage)
{
    if (!m_isLoading || m_isIndeterminate) {
        return;
    }

    m_progressBar->setValue(value);

    if (!subMessage.isEmpty()) {
        m_subMessageLabel->setText(subMessage);
    }
}

void LoadingOverlay::updateMessage(const QString& message)
{
    if (!m_isLoading) {
        return;
    }

    m_currentMessage = message;
    m_messageLabel->setText(message);
}

void LoadingOverlay::hideLoading()
{
    if (!m_isLoading) {
        return;
    }

    m_isLoading = false;
    m_elapsedTimer->stop();

    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(m_currentOpacity);
    m_fadeAnimation->setEndValue(0.0);
    m_fadeAnimation->start();
}

void LoadingOverlay::setOpacity(qreal opacity)
{
    m_currentOpacity = qBound(0.0, opacity, 1.0);
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(m_currentOpacity);
    }
    update();
}

void LoadingOverlay::setPulseGlow(int value)
{
    m_pulseGlowValue = value;
    update();
}

void LoadingOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bgColor(20, 20, 25, 220);
    painter.fillRect(rect(), bgColor);

    if (!m_contentWidget) {
        return;
    }

    QRect contentRect = m_contentWidget->geometry();
    QPainterPath path;
    path.addRoundedRect(contentRect, 16, 16);

    QLinearGradient gradient(contentRect.topLeft(), contentRect.bottomLeft());
    gradient.setColorAt(0, QColor(50, 50, 55, 250));
    gradient.setColorAt(1, QColor(35, 35, 40, 250));
    painter.fillPath(path, gradient);

    if (m_pulseGlowValue > 0) {
        painter.save();
        QPen glowPen(QColor(100, 150, 255, m_pulseGlowValue * 2), 2);
        painter.setPen(glowPen);
        painter.drawPath(path);
        painter.restore();
    }

    QPen borderPen(QColor(255, 255, 255, 20), 1);
    painter.setPen(borderPen);
    painter.drawPath(path);
}

void LoadingOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton ||
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier)) {
        emit cancelled();
        hideLoading();
    }
    event->accept();
}

void LoadingOverlay::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancelled();
        hideLoading();
    }
    event->accept();
}

bool LoadingOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        centerOnParent();
    }
    return QWidget::eventFilter(watched, event);
}

void LoadingOverlay::updateElapsedTime()
{
    if (!m_isLoading) {
        return;
    }

    qint64 elapsed = m_loadTimer.elapsed();
    int seconds = elapsed / 1000;
    int minutes = seconds / 60;
    seconds %= 60;

    QString timeStr;
    if (minutes > 0) {
        timeStr = QString("%1m %2s").arg(minutes).arg(seconds);
    } else {
        timeStr = QString("%1s").arg(seconds);
    }

    m_elapsedTimeLabel->setText(QString("Elapsed: %1").arg(timeStr));
}

void LoadingOverlay::startPulseAnimation()
{
    if (m_pulseAnimation && m_pulseAnimation->state() != QPropertyAnimation::Running) {
        m_pulseAnimation->start();
    }
}

void LoadingOverlay::onFadeInFinished()
{
    startPulseAnimation();
}

void LoadingOverlay::onFadeOutFinished()
{
    hide();
    stopAnimations();
}

void LoadingOverlay::centerOnParent()
{
    if (parentWidget()) {
        resize(parentWidget()->size());
        int x = (width() - m_contentWidget->width()) / 2;
        int y = (height() - m_contentWidget->height()) / 2;
        m_contentWidget->move(x, y);
    }
}

void LoadingOverlay::startAnimations()
{
    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();

    QTimer::singleShot(FADE_DURATION, this, &LoadingOverlay::startPulseAnimation);
}

void LoadingOverlay::stopAnimations()
{
    if (m_pulseAnimation) {
        m_pulseAnimation->stop();
    }
    m_pulseGlowValue = 0;
}

void LoadingOverlay::applyProfessionalStyling()
{
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            background: rgba(255, 255, 255, 0.05);
            border: none;
            border-radius: 3px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(100, 150, 255, 0.7),
                stop:1 rgba(150, 200, 255, 0.7));
            border-radius: 3px;
        }
        QProgressBar::chunk[indeterminate="true"] {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 transparent,
                stop:0.4 rgba(100, 150, 255, 0.7),
                stop:0.6 rgba(100, 150, 255, 0.7),
                stop:1 transparent);
        }
    )");
}