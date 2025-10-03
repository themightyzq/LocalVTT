#ifndef FOGAUTOSAVECONTROLLER_H
#define FOGAUTOSAVECONTROLLER_H

#include <QObject>
#include <QString>

class QTimer;
class MapDisplay;

// Handles autosaving fog state to a sidecar .fog file per map
class FogAutosaveController : public QObject {
    Q_OBJECT
public:
    explicit FogAutosaveController(QObject* parent = nullptr);

    void setMapDisplay(MapDisplay* display);
    void setCurrentMapPath(const QString& mapPath);
    void setAutosaveDelayMs(int ms);

    // Optional: load previously saved fog state from file
    void loadFromFile();
    // Force immediate save of current fog state
    void saveNow();

signals:
    void notify(const QString& message);

public slots:
    void onFogChanged();

private slots:
    void onAutosaveTimeout();

private:
    QString fogFilePath() const;

    MapDisplay* m_display {nullptr};
    QTimer* m_timer {nullptr};
    QString m_currentMapPath;
    bool m_dirty {false};
};

#endif // FOGAUTOSAVECONTROLLER_H
