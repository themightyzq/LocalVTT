#ifndef GRIDCONTROLLER_H
#define GRIDCONTROLLER_H

#include <QObject>
#include <QString>

class MainWindow;
class MapDisplay;
class QSlider;
class QLabel;
class QAction;
class GridOverlay;

class GridController : public QObject
{
    Q_OBJECT

public:
    explicit GridController(QObject *parent = nullptr);

    void attachToMainWindow(MainWindow* mainWindow);
    void setMapDisplay(MapDisplay* display);
    void setGridSizeSlider(QSlider* slider, QLabel* label);
    void setToggleAction(QAction* action);

    bool isGridEnabled() const { return m_gridEnabled; }
    int getCurrentGridSize() const;

public slots:
    void toggleGrid();
    void toggleGridType();
    void setStandardGrid();
    void showGridInfo();
    void openCalibration();
    void onGridSizeChanged(int value);
    void updateGridSizeSlider();

signals:
    void gridToggled(bool enabled);
    void gridSizeChanged(int size);
    void gridTypeChanged();
    void requestStatus(const QString& message, int timeout = 2000);

private:
    MainWindow* m_mainWindow;
    MapDisplay* m_mapDisplay;
    QSlider* m_gridSizeSlider;
    QLabel* m_gridSizeLabel;
    QAction* m_toggleGridAction;
    bool m_gridEnabled;
};

#endif // GRIDCONTROLLER_H