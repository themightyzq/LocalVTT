#ifndef TOOLOVERLAYWIDGET_H
#define TOOLOVERLAYWIDGET_H

#include <QWidget>
#include <QString>

class QLabel;

class ToolOverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit ToolOverlayWidget(QWidget* parent = nullptr);
    void setText(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_label;
};

#endif // TOOLOVERLAYWIDGET_H

