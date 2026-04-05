#ifndef KEYBOARDSHORTCUTSOVERLAY_H
#define KEYBOARDSHORTCUTSOVERLAY_H

#include <QWidget>

class KeyboardShortcutsOverlay : public QWidget {
    Q_OBJECT
public:
    explicit KeyboardShortcutsOverlay(QWidget* parent = nullptr);

    void showOverlay();
    void dismiss();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    struct ShortcutEntry {
        QString key;
        QString description;
    };

    struct ShortcutColumn {
        QString title;
        QList<ShortcutEntry> entries;
    };

    QList<ShortcutColumn> buildShortcutData() const;
    void drawColumn(QPainter& painter, const ShortcutColumn& col, int x, int y, int width) const;
};

#endif // KEYBOARDSHORTCUTSOVERLAY_H
