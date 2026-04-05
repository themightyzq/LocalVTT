#include "ui/KeyboardShortcutsOverlay.h"
#include <QPainter>
#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QApplication>

KeyboardShortcutsOverlay::KeyboardShortcutsOverlay(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    hide();
}

void KeyboardShortcutsOverlay::showOverlay()
{
    resize(parentWidget()->size());
    raise();
    show();
    setFocus();

    // Fade in
    auto* effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(effect);
    auto* anim = new QPropertyAnimation(effect, "opacity", this);
    anim->setDuration(150);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void KeyboardShortcutsOverlay::dismiss()
{
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect());
    if (!effect) {
        hide();
        return;
    }
    auto* anim = new QPropertyAnimation(effect, "opacity", this);
    anim->setDuration(150);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, this, &QWidget::hide);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void KeyboardShortcutsOverlay::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event)
    dismiss();
}

void KeyboardShortcutsOverlay::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    dismiss();
}

QList<KeyboardShortcutsOverlay::ShortcutColumn> KeyboardShortcutsOverlay::buildShortcutData() const
{
    QList<ShortcutColumn> columns;

    columns.append({"Map Navigation", {
        {"Scroll", "Pan vertically"},
        {"Shift+Scroll", "Pan horizontally"},
        {"Ctrl+Scroll", "Zoom in/out"},
        {"Space+Drag", "Pan (trackpad)"},
        {"+ / -", "Zoom in / out"},
        {"/", "Fit to view"},
        {"Ctrl+1/2/3", "Zoom 100/200/300%"},
        {"Ctrl+O", "Open map"},
    }});

    columns.append({"Fog Tools", {
        {"F", "Toggle Fog Mode"},
        {"H", "Reveal / Hide toggle"},
        {"[ ]", "Brush size down / up"},
        {"Ctrl+Z", "Undo fog change"},
        {"Ctrl+Y", "Redo fog change"},
    }});

    columns.append({"Display", {
        {"P", "Player View on/off"},
        {"G", "Grid on/off"},
        {"L", "Lighting on/off"},
        {"V", "Sync view to players"},
        {"Shift+V", "Reset player auto-fit"},
    }});

    columns.append({"Panels", {
        {"A", "Atmosphere panel"},
        {"B", "Map Browser"},
        {"F1", "This help"},
        {"F12", "Debug console"},
        {"Esc", "Cancel / deselect"},
    }});

    return columns;
}

void KeyboardShortcutsOverlay::drawColumn(QPainter& painter, const ShortcutColumn& col, int x, int y, int colWidth) const
{
    QFont headerFont = painter.font();
    headerFont.setPixelSize(14);
    headerFont.setBold(true);
    painter.setFont(headerFont);
    painter.setPen(QColor(74, 144, 226));
    painter.drawText(x, y, col.title);

    QFont entryFont = painter.font();
    entryFont.setPixelSize(12);
    entryFont.setBold(false);

    int rowY = y + 24;
    for (const auto& entry : col.entries) {
        // Key
        painter.setFont(entryFont);
        painter.setPen(QColor(180, 200, 255));
        painter.drawText(x, rowY, entry.key);

        // Description
        painter.setPen(QColor(200, 200, 200));
        painter.drawText(x + 110, rowY, entry.description);

        rowY += 20;
    }
}

void KeyboardShortcutsOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Dark semi-transparent background
    painter.fillRect(rect(), QColor(0, 0, 0, 215));

    auto columns = buildShortcutData();

    // Calculate card dimensions
    int colWidth = 240;
    int numCols = columns.size();
    int cardWidth = numCols * colWidth + 60;
    int cardHeight = 300;
    int cardX = (width() - cardWidth) / 2;
    int cardY = (height() - cardHeight) / 2;

    // Draw card background
    QRectF cardRect(cardX, cardY, cardWidth, cardHeight);
    painter.setBrush(QColor(40, 40, 40, 240));
    painter.setPen(QColor(74, 144, 226, 100));
    painter.drawRoundedRect(cardRect, 12, 12);

    // Title
    QFont titleFont = painter.font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(224, 224, 224));
    painter.drawText(cardRect.adjusted(0, 20, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "Keyboard Shortcuts");

    // Draw columns
    int startX = cardX + 30;
    int startY = cardY + 60;
    for (int i = 0; i < numCols; ++i) {
        drawColumn(painter, columns[i], startX + i * colWidth, startY, colWidth);
    }

    // Dismiss hint
    QFont hintFont = painter.font();
    hintFont.setPixelSize(11);
    hintFont.setBold(false);
    painter.setFont(hintFont);
    painter.setPen(QColor(128, 128, 128));
    painter.drawText(cardRect.adjusted(0, 0, 0, -16), Qt::AlignHCenter | Qt::AlignBottom, "Press any key to dismiss");
}
