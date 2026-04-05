#ifndef MAPBROWSERWIDGET_H
#define MAPBROWSERWIDGET_H

#include <QDockWidget>
#include <QListWidget>
#include <QStringList>
#include <QMenu>
#include <QDateTime>
#include <QPoint>

class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QToolButton;
class RecentFilesController;

// Dockable map browser with recent files, favorites, and folder browsing
// Provides quick access to maps with thumbnail previews
class MapBrowserWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit MapBrowserWidget(QWidget* parent = nullptr);
    ~MapBrowserWidget() override;

    // Set the recent files controller to get recent file list
    void setRecentFilesController(RecentFilesController* controller);

    // Get current browse directory
    QString browseDirectory() const { return m_browseDirectory; }

signals:
    void mapSelected(const QString& filePath);

public slots:
    void refreshRecentFiles();
    void setBrowseDirectory(const QString& path);

private slots:
    void onRecentItemClicked(QListWidgetItem* item);
    void onFavoriteItemClicked(QListWidgetItem* item);
    void onBrowseItemClicked(QListWidgetItem* item);
    void onBrowseItemDoubleClicked(QListWidgetItem* item);
    void onBrowseButtonClicked();
    void onParentDirectoryClicked();
    void onRefreshClicked();
    void onThumbnailReady(const QString& filePath, const QPixmap& thumbnail);
    void onSearchTextChanged(const QString& text);
    void onAddToFavoritesClicked();
    void onRemoveFromFavoritesClicked();

    // Context menu slots
    void onBrowseContextMenu(const QPoint& pos);
    void onRecentContextMenu(const QPoint& pos);
    void onFavoritesContextMenu(const QPoint& pos);
    void showContextMenuForFile(const QString& filePath, const QPoint& globalPos, bool isInFavorites);

    // Context menu actions
    void revealInFinder(const QString& filePath);
    void copyFilePath(const QString& filePath);
    void renameFile(const QString& filePath);
    void deleteFile(const QString& filePath);
    void showFileInfo(const QString& filePath);

    // Sorting and view mode
    void onSortChanged(int index);
    void onViewModeToggled();

    // Folder bookmarks
    void onBookmarkFolderClicked();
    void onBookmarksMenuRequested();
    void goToBookmarkedFolder(const QString& path);
    void removeBookmarkedFolder(const QString& path);

    // Batch operations
    void onBatchAddToFavorites();
    void onBatchDelete();

    // Keyboard navigation
    void onBrowseItemActivated(QListWidgetItem* item);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void loadBrowseDirectory();
    void updateRecentFilesSection();
    void updateFavoritesSection();
    void updateBrowseSection();
    void addFileToList(QListWidget* list, const QString& filePath, bool showFavoriteStar = false);
    void filterBrowseList(const QString& filter);

    // Favorites management
    void loadFavorites();
    void saveFavorites();
    bool isFavorite(const QString& filePath) const;
    void addFavorite(const QString& filePath);
    void removeFavorite(const QString& filePath);

    // Supported file extensions
    bool isSupportedFile(const QString& filePath) const;

    // Recursive directory scanning for search
    void scanDirectoryRecursive(const QString& path, int depth);

    // Sorting
    void sortBrowseItems();
    void loadFolderBookmarks();
    void saveFolderBookmarks();

    // View mode
    void applyViewMode();

    // File count update
    void updateFileCount();

    // Drag support
    void setupDragDrop();

    QWidget* m_mainWidget;
    QVBoxLayout* m_mainLayout;

    // Recent files section
    QLabel* m_recentLabel;
    QListWidget* m_recentList;

    // Favorites section
    QLabel* m_favoritesLabel;
    QListWidget* m_favoritesList;
    QStringList m_favorites;  // Stored favorite paths

    // Browse section
    QLabel* m_browseLabel;
    QWidget* m_pathWidget;
    QPushButton* m_parentButton;
    QPushButton* m_refreshButton;
    QLineEdit* m_pathEdit;
    QPushButton* m_browseButton;

    // Search and controls bar
    QWidget* m_searchWidget;
    QLineEdit* m_searchEdit;
    QPushButton* m_favoriteButton;  // Star button to add selected to favorites

    // Sort and view controls
    QWidget* m_controlsWidget;
    QComboBox* m_sortCombo;
    QToolButton* m_viewModeButton;
    QToolButton* m_bookmarkButton;
    QLabel* m_fileCountLabel;

    QListWidget* m_browseList;

    RecentFilesController* m_recentFilesController;
    QString m_browseDirectory;
    QString m_currentFilter;  // Current search filter

    // Track items by path for thumbnail updates
    QHash<QString, QListWidgetItem*> m_itemsByPath;

    // Track all browse items for filtering and sorting
    struct BrowseItem {
        QString filePath;
        QString fileName;
        bool isDirectory;
        qint64 fileSize;
        QDateTime modifiedDate;
    };
    QList<BrowseItem> m_allBrowseItems;

    // Sorting state
    enum class SortMode { Name, DateModified, Size };
    SortMode m_sortMode = SortMode::Name;
    bool m_sortAscending = true;

    // View mode state
    bool m_gridView = true;  // true = grid/icon mode, false = list mode

    // Folder bookmarks
    QStringList m_folderBookmarks;

    // Drag support
    QPoint m_dragStartPos;
};

#endif // MAPBROWSERWIDGET_H
