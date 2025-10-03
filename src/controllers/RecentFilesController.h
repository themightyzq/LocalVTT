#ifndef RECENTFILESCONTROLLER_H
#define RECENTFILESCONTROLLER_H

#include <QObject>
#include <QList>
#include <QString>

class QMenu;
class QAction;
class QIcon;

// Manages the Recent Files menu and persistence via SettingsManager
class RecentFilesController : public QObject {
    Q_OBJECT
public:
    explicit RecentFilesController(QObject* parent = nullptr);

    void attach(QMenu* recentMenu,
                const QList<QAction*>& recentFileActions,
                QAction* clearAction,
                int maxRecentFiles = 10);

    void updateMenu();
    void addToRecent(const QString& filePath);

signals:
    void openFileRequested(const QString& filePath);

public slots:
    void handleOpenRecent();
    void clearRecent();

private:
    QMenu* m_recentMenu {nullptr};
    QList<QAction*> m_actions;
    QAction* m_clearAction {nullptr};
    int m_maxRecent {10};

    void setActionsEnabled(bool hasFiles);
    QIcon getFileTypeIcon(const QString& extension) const;
};

#endif // RECENTFILESCONTROLLER_H
