#ifndef TABSCONTROLLER_H
#define TABSCONTROLLER_H

#include <QObject>
#include <QList>
#include <QString>

class QTabWidget;
class MapDisplay;
class MapSession;

class TabsController : public QObject {
    Q_OBJECT
public:
    explicit TabsController(QObject* parent = nullptr);

    void attach(QTabWidget* tabs, MapDisplay* display, int maxTabs = 10);
    void loadMapFile(const QString& path);

    // Get current map session
    MapSession* getCurrentSession() const;

signals:
    void requestShowProgress(const QString& fileName, qint64 fileSize);
    void requestHideProgress();
    void requestStatus(const QString& message, int ms);
    void requestAddRecent(const QString& path);
    void currentMapPathChanged(const QString& path);
    void uiChanged();      // grid/fog/undo/redo indicators should update
    void sceneChanged();   // player window may need refresh

public slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void setCurrentIndex(int index) { switchToTab(index); }
    void closeIndex(int index) { closeTab(index); }

private:
    void createNewTab(const QString& filePath);
    void switchToTab(int index);
    void closeTab(int index);
    QString shortTitle(const QString& filePath) const;

    QTabWidget* m_tabs {nullptr};
    MapDisplay* m_display {nullptr};
    QList<MapSession*> m_sessions;
    int m_currentIndex {-1};
    int m_maxTabs {10};
};

#endif // TABSCONTROLLER_H
