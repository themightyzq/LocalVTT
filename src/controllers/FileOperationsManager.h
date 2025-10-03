#ifndef FILEOPERATIONSMANAGER_H
#define FILEOPERATIONSMANAGER_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>

class MainWindow;
class MapDisplay;
class QProgressDialog;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QWidget;
class QPropertyAnimation;

class FileOperationsManager : public QObject
{
    Q_OBJECT

public:
    explicit FileOperationsManager(MainWindow* mainWindow, MapDisplay* mapDisplay, QObject* parent = nullptr);
    ~FileOperationsManager() = default;

    void loadMap();
    void loadMapFile(const QString& path);
    void loadMapFromCommandLine(const QString& path);

    void handleDragEnterEvent(QDragEnterEvent* event);
    void handleDragMoveEvent(QDragMoveEvent* event);
    void handleDragLeaveEvent(QDragLeaveEvent* event);
    void handleDropEvent(QDropEvent* event);

    QString getFogFilePath(const QString& mapPath) const;
    void loadFogState(const QString& mapPath);
    void saveFogState();
    void quickSaveFogState();
    void quickRestoreFogState();

signals:
    void mapLoaded(const QString& filePath);
    void fogStateLoaded(const QString& mapPath);
    void fogStateSaved();
    void loadProgressUpdate(qint64 bytesRead, qint64 totalBytes);
    void dropAnimationRequested(bool entering);
    void autosaveNotificationRequested(const QString& message);

private slots:
    void handleLoadProgress(qint64 bytesRead, qint64 totalBytes);
    void animateDropFeedback(bool entering);

private:
    void showLoadProgress(const QString& fileName, qint64 fileSize);
    void hideLoadProgress();
    void createDropOverlay();
    void scheduleFogSave();
    void showAutosaveNotification(const QString& message);

    MainWindow* m_mainWindow;
    MapDisplay* m_mapDisplay;
    QProgressDialog* m_progressDialog;
    QWidget* m_dropOverlay;
    QPropertyAnimation* m_dropAnimation;
    QElapsedTimer m_loadTimer;
    bool m_isDragging;
    QString m_currentMapPath;
    QString m_quickSavePath;
};

#endif // FILEOPERATIONSMANAGER_H