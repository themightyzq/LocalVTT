#include "graphics/ToolOverlayWidget.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>

ToolOverlayWidget::ToolOverlayWidget(QWidget* parent)
    : QWidget(parent)
    , m_label(new QLabel(this))
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAutoFillBackground(false);
    m_label->setStyleSheet("QLabel { color: #FFFFFF; font-weight: 600; }");
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->addWidget(m_label);
    setStyleSheet("background: rgba(0,0,0,0.45); border-radius: 10px;");
    hide();
}

void ToolOverlayWidget::setText(const QString& text)
{
    m_label->setText(text);
    if (!text.isEmpty()) show(); else hide();
}

void ToolOverlayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStyleOption opt; opt.QStyleOption::initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
