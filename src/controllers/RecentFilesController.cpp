#include "RecentFilesController.h"
#include "utils/SettingsManager.h"

#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QKeySequence>
#include <QDateTime>
#include <QApplication>
#include <QStyle>
#include <QFont>
#include <QIcon>

RecentFilesController::RecentFilesController(QObject* parent)
    : QObject(parent) {}

void RecentFilesController::attach(QMenu* recentMenu,
                                   const QList<QAction*>& recentFileActions,
                                   QAction* clearAction,
                                   int maxRecentFiles)
{
    m_recentMenu = recentMenu;
    m_actions = recentFileActions;
    m_clearAction = clearAction;
    m_maxRecent = maxRecentFiles;

    // Hook up action triggers to our slot
    for (QAction* a : m_actions) {
        connect(a, &QAction::triggered, this, &RecentFilesController::handleOpenRecent);
    }
    if (m_clearAction) {
        connect(m_clearAction, &QAction::triggered, this, &RecentFilesController::clearRecent);
    }

    updateMenu();
}

void RecentFilesController::updateMenu()
{
    if (!m_recentMenu) return;

    QStringList recentFiles = SettingsManager::instance().loadRecentFiles();

    // Remove non-existent files and update the list
    QStringList validFiles;
    for (const QString& filePath : recentFiles) {
        if (QFileInfo::exists(filePath)) {
            validFiles.append(filePath);
        }
    }

    // Update settings if we removed any invalid files
    if (validFiles.size() != recentFiles.size()) {
        SettingsManager::instance().saveRecentFiles(validFiles);
    }

    const int num = qMin(validFiles.size(), m_maxRecent);

    for (int i = 0; i < num; ++i) {
        const QString& filePath = validFiles[i];
        QFileInfo fi(filePath);
        const QString shortcutKey = (i < 9) ? QString::number(i + 1) : "0";

        // Get file metadata
        QString fileName = fi.fileName();
        qint64 fileSize = fi.size();
        QDateTime lastModified = fi.lastModified();

        // Format file size
        QString sizeStr;
        if (fileSize < 1024) {
            sizeStr = QString("%1 B").arg(fileSize);
        } else if (fileSize < 1024 * 1024) {
            sizeStr = QString("%1 KB").arg(fileSize / 1024);
        } else {
            sizeStr = QString("%1 MB").arg(fileSize / (1024 * 1024));
        }

        // Format last modified date (show relative time for recent files)
        QString dateStr;
        QDateTime now = QDateTime::currentDateTime();
        qint64 secondsAgo = lastModified.secsTo(now);

        if (secondsAgo < 3600) { // Less than 1 hour
            int minutes = qMax(1, static_cast<int>(secondsAgo / 60));
            dateStr = QString("%1m ago").arg(minutes);
        } else if (secondsAgo < 86400) { // Less than 1 day
            int hours = secondsAgo / 3600;
            dateStr = QString("%1h ago").arg(hours);
        } else if (secondsAgo < 604800) { // Less than 1 week
            int days = secondsAgo / 86400;
            dateStr = QString("%1d ago").arg(days);
        } else {
            dateStr = lastModified.toString("MMM d");
        }

        // Create display text with filename only (clean look)
        const QString displayText = QString("&%1 %2").arg(shortcutKey, fileName);

        // Set action properties
        m_actions[i]->setText(displayText);
        m_actions[i]->setData(filePath);
        m_actions[i]->setVisible(true);
        m_actions[i]->setStatusTip(QString("%1 (%2, %3)").arg(filePath, sizeStr, dateStr));
        m_actions[i]->setToolTip(QString("%1\n%2\nSize: %3\nLast modified: %4")
            .arg(fileName, filePath, sizeStr, lastModified.toString("MMM d, yyyy at h:mm AP")));

        // Set keyboard shortcuts (Ctrl+1 through Ctrl+9)
        if (i < 9) {
            m_actions[i]->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)));
        } else {
            m_actions[i]->setShortcut(QKeySequence());
        }

        // Add file type icon
        QIcon fileIcon = getFileTypeIcon(fi.suffix().toLower());
        m_actions[i]->setIcon(fileIcon);

        // Mark the most recently used file in tooltip
        if (i == 0) {
            m_actions[i]->setToolTip(QString("★ Most Recent: %1\n%2\nSize: %3\nLast modified: %4")
                .arg(fileName, filePath, sizeStr, lastModified.toString("MMM d, yyyy at h:mm AP")));
        }
    }

    // Hide unused actions
    for (int i = num; i < m_actions.size(); ++i) {
        m_actions[i]->setVisible(false);
    }

    setActionsEnabled(!validFiles.isEmpty());
}

void RecentFilesController::addToRecent(const QString& filePath)
{
    QStringList recent = SettingsManager::instance().loadRecentFiles();
    recent.removeAll(filePath);
    recent.prepend(filePath);
    while (recent.size() > m_maxRecent) recent.removeLast();
    SettingsManager::instance().saveRecentFiles(recent);
    updateMenu();
}

void RecentFilesController::handleOpenRecent()
{
    if (QAction* a = qobject_cast<QAction*>(sender())) {
        const QString filePath = a->data().toString();
        if (filePath.isEmpty()) return;
        if (QFileInfo::exists(filePath)) {
            // Move opened file to top of list
            addToRecent(filePath);
            emit openFileRequested(filePath);
        } else {
            // Remove non-existent file and refresh
            QStringList recent = SettingsManager::instance().loadRecentFiles();
            recent.removeAll(filePath);
            SettingsManager::instance().saveRecentFiles(recent);
            updateMenu();
        }
    }
}

void RecentFilesController::clearRecent()
{
    SettingsManager::instance().saveRecentFiles(QStringList{});
    updateMenu();
}

void RecentFilesController::setActionsEnabled(bool hasFiles)
{
    if (m_recentMenu) m_recentMenu->setEnabled(hasFiles);
    if (m_clearAction) m_clearAction->setEnabled(hasFiles);
}

QIcon RecentFilesController::getFileTypeIcon(const QString& extension) const
{
    // Get appropriate file type icon based on extension
    QStyle* style = QApplication::style();

    // VTT file extensions
    QStringList vttExtensions = {"dd2vtt", "uvtt", "df2vtt"};
    if (vttExtensions.contains(extension)) {
        // Use a table/grid icon for VTT files (which contain map and grid data)
        return style->standardIcon(QStyle::SP_FileDialogDetailedView);
    }

    // Image file extensions
    QStringList imageExtensions = {"png", "jpg", "jpeg", "webp", "bmp", "gif", "tiff", "tif"};
    if (imageExtensions.contains(extension)) {
        // Use a generic file icon for images (Qt doesn't have a specific image icon)
        return style->standardIcon(QStyle::SP_FileIcon);
    }

    // Default file icon for unknown extensions
    return style->standardIcon(QStyle::SP_FileIcon);
}
