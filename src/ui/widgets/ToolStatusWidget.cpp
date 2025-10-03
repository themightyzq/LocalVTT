#include "ui/widgets/ToolStatusWidget.h"
#include "utils/AnimationHelper.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QFontMetrics>

namespace Colors {
    const QColor BgPrimary{"#1a1a1a"};
    const QColor BgSecondary{"#242424"};
    const QColor AccentPrimary{"#4A90E2"};
    const QColor AccentDanger{"#E74C3C"};
    const QColor TextPrimary{"#E0E0E0"};
    const QColor TextSecondary{"#B0B0B0"};
    const QColor BorderColor{"#333333"};
}

ToolStatusWidget::ToolStatusWidget(QWidget* parent)
    : QWidget(parent)
    , m_iconLabel(nullptr)
    , m_nameLabel(nullptr)
    , m_hintLabel(nullptr)
    , m_layout(nullptr)
    , m_fadeAnimation(nullptr)
    , m_opacityEffect(nullptr)
    , m_updateTimer(nullptr)
    , m_currentTool(ToolType::Pointer)
    , m_currentFogMode(FogToolMode::UnifiedFog)
    , m_animatingTransition(false)
{
    setupUI();
    setupAnimations();
    updateContent();
}

void ToolStatusWidget::setupUI()
{
    setFixedHeight(32);
    setMinimumWidth(300);
    
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(12, 4, 12, 4);
    m_layout->setSpacing(8);
    
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet(QString(
        "QLabel {"
        "    color: %1;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "}"
    ).arg(Colors::AccentPrimary.name()));
    
    QFrame* separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::VLine);
    separator1->setFixedSize(1, 20);
    separator1->setStyleSheet(QString("QFrame { background-color: %1; }").arg(Colors::BorderColor.name()));
    
    m_nameLabel = new QLabel(this);
    m_nameLabel->setMinimumWidth(80);
    m_nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_nameLabel->setStyleSheet(QString(
        "QLabel {"
        "    color: %1;"
        "    font-size: 13px;"
        "    font-weight: 600;"
        "    background: transparent;"
        "}"
    ).arg(Colors::TextPrimary.name()));
    
    QFrame* separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::VLine);
    separator2->setFixedSize(1, 20);
    separator2->setStyleSheet(QString("QFrame { background-color: %1; }").arg(Colors::BorderColor.name()));
    
    m_hintLabel = new QLabel(this);
    m_hintLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_hintLabel->setStyleSheet(QString(
        "QLabel {"
        "    color: %1;"
        "    font-size: 12px;"
        "    font-style: italic;"
        "    background: transparent;"
        "}"
    ).arg(Colors::TextSecondary.name()));
    
    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(separator1);
    m_layout->addWidget(m_nameLabel);
    m_layout->addWidget(separator2);
    m_layout->addWidget(m_hintLabel, 1);
    
    setLayout(m_layout);
}

void ToolStatusWidget::setupAnimations()
{
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(1.0);
        setGraphicsEffect(m_opacityEffect);

        m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
        if (m_fadeAnimation) {
            // Use consistent animation timing per CLAUDE.md (150ms)
            m_fadeAnimation->setDuration(AnimationHelper::STANDARD_DURATION);
            m_fadeAnimation->setEasingCurve(AnimationHelper::smoothEasing());

            connect(m_fadeAnimation, &QPropertyAnimation::finished,
                    this, &ToolStatusWidget::onAnimationFinished);
        }
    }

    m_updateTimer = new QTimer(this);
    if (m_updateTimer) {
        m_updateTimer->setSingleShot(true);
        m_updateTimer->setInterval(UPDATE_DELAY);

        connect(m_updateTimer, &QTimer::timeout,
                this, &ToolStatusWidget::onUpdateTimer);
    }
}

void ToolStatusWidget::setCurrentTool(ToolType tool)
{
    if (m_currentTool == tool && !m_animatingTransition) {
        return;
    }
    
    m_currentTool = tool;
    m_customHint.clear();
    startFadeTransition();
}

void ToolStatusWidget::setFogToolMode(FogToolMode mode)
{
    if (m_currentFogMode == mode && !m_animatingTransition) {
        return;
    }
    
    m_currentFogMode = mode;
    
    if (m_currentTool == ToolType::FogBrush) {
        startFadeTransition();
    }
}

void ToolStatusWidget::updateHintText(const QString& hint)
{
    if (m_customHint == hint) {
        return;
    }
    
    m_customHint = hint;
    
    if (!m_animatingTransition) {
        m_hintLabel->setText(hint.isEmpty() ? getToolHint(m_currentTool) : hint);
    }
}

void ToolStatusWidget::onToolChanged(ToolType tool)
{
    setCurrentTool(tool);
}

void ToolStatusWidget::onFogToolModeChanged(FogToolMode mode)
{
    setFogToolMode(mode);
}

void ToolStatusWidget::startFadeTransition()
{
    if (m_animatingTransition || !m_fadeAnimation) {
        return;
    }

    m_animatingTransition = true;

    // Create a smooth fade transition for tool changes
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.3);
    m_fadeAnimation->start();

    // Add a subtle highlight flash to draw attention to the change
    AnimationHelper::animateToolHighlight(this, Colors::AccentPrimary);
}

void ToolStatusWidget::onAnimationFinished()
{
    if (m_animatingTransition && m_fadeAnimation && m_fadeAnimation->endValue().toReal() < 1.0) {
        if (m_updateTimer) {
            m_updateTimer->start();
        }
    } else {
        m_animatingTransition = false;
    }
}

void ToolStatusWidget::onUpdateTimer()
{
    updateContent();

    if (m_fadeAnimation) {
        m_fadeAnimation->setStartValue(0.3);
        m_fadeAnimation->setEndValue(1.0);
        m_fadeAnimation->start();
    }
}

void ToolStatusWidget::updateContent()
{
    // Defensive checks to prevent crashes during initialization
    if (!m_iconLabel || !m_nameLabel || !m_hintLabel) {
        return;
    }

    m_iconLabel->setText(getToolIcon(m_currentTool));

    QString toolName = getToolName(m_currentTool);
    if (m_currentTool == ToolType::FogBrush) {
        toolName += QString(" (%1)").arg(getFogModeText(m_currentFogMode));
    }
    m_nameLabel->setText(toolName);

    QString hint = m_customHint.isEmpty() ? getToolHint(m_currentTool) : m_customHint;
    m_hintLabel->setText(hint);

    adjustSize();
}

QString ToolStatusWidget::getToolIcon(ToolType tool) const
{
    switch (tool) {
        case ToolType::Pointer: return "👆";
        case ToolType::FogBrush: return "🖌";
        case ToolType::FogRectangle: return "⬜";
        default: return "❓";
    }
}

QString ToolStatusWidget::getToolName(ToolType tool) const
{
    switch (tool) {
        case ToolType::Pointer: return "Pointer";
        case ToolType::FogBrush: return "Fog Brush";
        case ToolType::FogRectangle: return "Fog Rectangle";
        default: return "Unknown";
    }
}

QString ToolStatusWidget::getToolHint(ToolType tool) const
{
    switch (tool) {
        case ToolType::Pointer:
            return "Double-click anywhere for beacon • Middle-click drag to pan";
        case ToolType::FogBrush:
            return "Click/drag to reveal fog • Adjust brush size in toolbar";
        case ToolType::FogRectangle:
            return "Drag to reveal rectangular area • Middle-click to pan";
        default:
            return "";
    }
}

QString ToolStatusWidget::getFogModeText(FogToolMode mode) const
{
    switch (mode) {
        case FogToolMode::UnifiedFog: return "Unified Fog Tool";
        case FogToolMode::DrawPen: return "Drawing Pen";
        case FogToolMode::DrawEraser: return "Drawing Eraser";
        default: return "Unknown";
    }
}

void ToolStatusWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    QRect widgetRect = rect();
    
    painter.setBrush(QBrush(Colors::BgSecondary));
    painter.setPen(QPen(Colors::BorderColor, 1));
    painter.drawRoundedRect(widgetRect, 4, 4);
    
    QLinearGradient highlight(0, 0, 0, height());
    highlight.setColorAt(0, QColor(255, 255, 255, 10));
    highlight.setColorAt(1, QColor(255, 255, 255, 0));
    
    painter.setBrush(QBrush(highlight));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(widgetRect.adjusted(1, 1, -1, -height()/2), 3, 3);
}

QSize ToolStatusWidget::sizeHint() const
{
    if (!m_nameLabel || !m_hintLabel) {
        return QSize(300, 32);
    }
    
    QFontMetrics nameFM(m_nameLabel->font());
    QFontMetrics hintFM(m_hintLabel->font());
    
    int nameWidth = nameFM.horizontalAdvance(m_nameLabel->text());
    int hintWidth = hintFM.horizontalAdvance(m_hintLabel->text());
    
    int totalWidth = 24 + 8 + 1 + 8 + nameWidth + 8 + 1 + 8 + hintWidth + 24;
    
    return QSize(qMax(300, totalWidth), 32);
}

QSize ToolStatusWidget::minimumSizeHint() const
{
    return QSize(300, 32);
}

qreal ToolStatusWidget::opacity() const
{
    return m_opacityEffect ? m_opacityEffect->opacity() : 1.0;
}

void ToolStatusWidget::setOpacity(qreal opacity)
{
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(opacity);
    }
}