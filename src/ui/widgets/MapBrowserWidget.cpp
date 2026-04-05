#include "MapBrowserWidget.h"
#include "ThumbnailCache.h"
#include "controllers/RecentFilesController.h"
#include "utils/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QPainter>
#include <QSettings>
#include <QProcess>
#include <QClipboard>
#include <QGuiApplication>
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QRegularExpression>
#include <QComboBox>
#include <QToolButton>
#include <QMimeData>
#include <QDrag>
#include <QEvent>
#include <QKeyEvent>
#include <QToolTip>
#include <algorithm>

MapBrowserWidget::MapBrowserWidget(QWidget* parent)
    : QDockWidget("Map Browser", parent)
    , m_mainWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_recentLabel(nullptr)
    , m_recentList(nullptr)
    , m_favoritesLabel(nullptr)
    , m_favoritesList(nullptr)
    , m_browseLabel(nullptr)
    , m_pathWidget(nullptr)
    , m_parentButton(nullptr)
    , m_refreshButton(nullptr)
    , m_pathEdit(nullptr)
    , m_browseButton(nullptr)
    , m_searchWidget(nullptr)
    , m_searchEdit(nullptr)
    , m_favoriteButton(nullptr)
    , m_controlsWidget(nullptr)
    , m_sortCombo(nullptr)
    , m_viewModeButton(nullptr)
    , m_bookmarkButton(nullptr)
    , m_fileCountLabel(nullptr)
    , m_browseList(nullptr)
    , m_recentFilesController(nullptr)
{
    setObjectName("MapBrowserDock");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

    // Load favorites and folder bookmarks before setting up UI
    loadFavorites();
    loadFolderBookmarks();

    // Load view mode preference
    QSettings settings;
    m_gridView = settings.value("mapBrowser/gridView", true).toBool();
    m_sortMode = static_cast<SortMode>(settings.value("mapBrowser/sortMode", 0).toInt());

    setupUI();

    // Connect to thumbnail cache for async updates
    connect(&ThumbnailCache::instance(), &ThumbnailCache::thumbnailReady,
            this, &MapBrowserWidget::onThumbnailReady);

    // Load saved browse directory (reuse settings object from above)
    m_browseDirectory = settings.value("mapBrowser/directory", "").toString();
    if (!m_browseDirectory.isEmpty() && QDir(m_browseDirectory).exists()) {
        loadBrowseDirectory();
    }
}

MapBrowserWidget::~MapBrowserWidget()
{
    // Save browse directory
    QSettings settings;
    settings.setValue("mapBrowser/directory", m_browseDirectory);

    // Favorites are saved immediately when modified
}

void MapBrowserWidget::setupUI()
{
    m_mainWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(m_mainWidget);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);

    // Common list style
    const QString listStyle =
        "QListWidget { background: #252525; border: 1px solid #3A3A3A; border-radius: 4px; }"
        "QListWidget::item { padding: 4px; border-radius: 3px; }"
        "QListWidget::item:hover { background: #3A3A3A; }"
        "QListWidget::item:selected { background: #4A90E2; }";

    // === Recent Files Section ===
    m_recentLabel = new QLabel("RECENT FILES");
    m_recentLabel->setStyleSheet("font-weight: bold; color: #808080; font-size: 11px;");
    m_mainLayout->addWidget(m_recentLabel);

    m_recentList = new QListWidget();
    m_recentList->setViewMode(QListView::ListMode);
    m_recentList->setIconSize(QSize(48, 48));
    m_recentList->setSpacing(2);
    m_recentList->setMaximumHeight(150);
    m_recentList->setStyleSheet(listStyle);
    m_recentList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_recentList, &QListWidget::itemClicked,
            this, &MapBrowserWidget::onRecentItemClicked);
    connect(m_recentList, &QWidget::customContextMenuRequested,
            this, &MapBrowserWidget::onRecentContextMenu);
    m_mainLayout->addWidget(m_recentList);

    // === Favorites Section ===
    m_favoritesLabel = new QLabel("★ FAVORITES");
    m_favoritesLabel->setStyleSheet("font-weight: bold; color: #E2A94A; font-size: 11px;");
    m_mainLayout->addWidget(m_favoritesLabel);

    m_favoritesList = new QListWidget();
    m_favoritesList->setViewMode(QListView::ListMode);
    m_favoritesList->setIconSize(QSize(48, 48));
    m_favoritesList->setSpacing(2);
    m_favoritesList->setMaximumHeight(150);
    m_favoritesList->setStyleSheet(listStyle);
    m_favoritesList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_favoritesList, &QListWidget::itemClicked,
            this, &MapBrowserWidget::onFavoriteItemClicked);
    connect(m_favoritesList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                QString filePath = item->data(Qt::UserRole).toString();
                if (!filePath.isEmpty() && QFile::exists(filePath)) {
                    emit mapSelected(filePath);
                }
            });
    connect(m_favoritesList, &QWidget::customContextMenuRequested,
            this, &MapBrowserWidget::onFavoritesContextMenu);
    m_mainLayout->addWidget(m_favoritesList);

    // === Browse Folder Section ===
    m_browseLabel = new QLabel("BROWSE FOLDER");
    m_browseLabel->setStyleSheet("font-weight: bold; color: #808080; font-size: 11px;");
    m_mainLayout->addWidget(m_browseLabel);

    // Path display
    m_pathEdit = new QLineEdit();
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText("No folder selected");
    m_pathEdit->setStyleSheet(
        "QLineEdit { background: #252525; border: 1px solid #3A3A3A; border-radius: 3px; padding: 4px; }"
    );
    m_mainLayout->addWidget(m_pathEdit);

    // Button bar: Up, Open, Star
    m_pathWidget = new QWidget();
    QHBoxLayout* pathLayout = new QHBoxLayout(m_pathWidget);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->setSpacing(4);

    m_parentButton = new QPushButton("↑");
    m_parentButton->setFixedWidth(32);
    m_parentButton->setToolTip("Go to parent folder");
    m_parentButton->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: bold; }"
    );
    connect(m_parentButton, &QPushButton::clicked,
            this, &MapBrowserWidget::onParentDirectoryClicked);
    pathLayout->addWidget(m_parentButton);

    m_refreshButton = new QPushButton("⟳");
    m_refreshButton->setFixedWidth(32);
    m_refreshButton->setToolTip("Refresh folder contents");
    m_refreshButton->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: bold; }"
    );
    connect(m_refreshButton, &QPushButton::clicked,
            this, &MapBrowserWidget::onRefreshClicked);
    pathLayout->addWidget(m_refreshButton);

    m_browseButton = new QPushButton("Open...");
    m_browseButton->setToolTip("Select a folder to browse");
    connect(m_browseButton, &QPushButton::clicked,
            this, &MapBrowserWidget::onBrowseButtonClicked);
    pathLayout->addWidget(m_browseButton);

    m_favoriteButton = new QPushButton("★ Favorite");
    m_favoriteButton->setToolTip("Add selected map to favorites");
    m_favoriteButton->setStyleSheet(
        "QPushButton { color: #E2A94A; }"
        "QPushButton:hover { background: #3A3A3A; }"
        "QPushButton:disabled { color: #505050; }"
    );
    connect(m_favoriteButton, &QPushButton::clicked,
            this, &MapBrowserWidget::onAddToFavoritesClicked);
    pathLayout->addWidget(m_favoriteButton);

    pathLayout->addStretch();  // Push buttons to the left

    m_mainLayout->addWidget(m_pathWidget);

    // Search bar
    m_searchWidget = new QWidget();
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchWidget);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(4);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search maps (includes subfolders)...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background: #252525; border: 1px solid #3A3A3A; border-radius: 3px; padding: 4px; }"
    );
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &MapBrowserWidget::onSearchTextChanged);
    searchLayout->addWidget(m_searchEdit, 1);

    m_mainLayout->addWidget(m_searchWidget);

    // Controls bar: Sort, View Mode, Bookmark, File Count
    m_controlsWidget = new QWidget();
    QHBoxLayout* controlsLayout = new QHBoxLayout(m_controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(4);

    // Sort combo
    m_sortCombo = new QComboBox();
    m_sortCombo->addItem("Name ↑", QVariant::fromValue(0));
    m_sortCombo->addItem("Name ↓", QVariant::fromValue(1));
    m_sortCombo->addItem("Date ↑", QVariant::fromValue(2));
    m_sortCombo->addItem("Date ↓", QVariant::fromValue(3));
    m_sortCombo->addItem("Size ↑", QVariant::fromValue(4));
    m_sortCombo->addItem("Size ↓", QVariant::fromValue(5));
    m_sortCombo->setToolTip("Sort files");
    m_sortCombo->setCurrentIndex(static_cast<int>(m_sortMode) * 2 + (m_sortAscending ? 0 : 1));
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MapBrowserWidget::onSortChanged);
    controlsLayout->addWidget(m_sortCombo);

    // View mode toggle button
    m_viewModeButton = new QToolButton();
    m_viewModeButton->setText(m_gridView ? "☷" : "☰");
    m_viewModeButton->setToolTip(m_gridView ? "Switch to list view" : "Switch to grid view");
    m_viewModeButton->setStyleSheet("QToolButton { font-size: 14px; padding: 4px 8px; }");
    connect(m_viewModeButton, &QToolButton::clicked,
            this, &MapBrowserWidget::onViewModeToggled);
    controlsLayout->addWidget(m_viewModeButton);

    // Bookmark folder button
    m_bookmarkButton = new QToolButton();
    m_bookmarkButton->setText("📁");
    m_bookmarkButton->setToolTip("Folder bookmarks");
    m_bookmarkButton->setPopupMode(QToolButton::InstantPopup);
    m_bookmarkButton->setStyleSheet("QToolButton { font-size: 14px; padding: 4px 8px; }");
    connect(m_bookmarkButton, &QToolButton::clicked,
            this, &MapBrowserWidget::onBookmarksMenuRequested);
    controlsLayout->addWidget(m_bookmarkButton);

    controlsLayout->addStretch();

    // File count label
    m_fileCountLabel = new QLabel("0 files");
    m_fileCountLabel->setStyleSheet("color: #808080; font-size: 11px;");
    controlsLayout->addWidget(m_fileCountLabel);

    m_mainLayout->addWidget(m_controlsWidget);

    // Browse list with thumbnails
    m_browseList = new QListWidget();
    m_browseList->setViewMode(m_gridView ? QListView::IconMode : QListView::ListMode);
    m_browseList->setIconSize(m_gridView ? QSize(96, 96) : QSize(48, 48));
    m_browseList->setGridSize(m_gridView ? QSize(110, 130) : QSize(-1, -1));
    m_browseList->setSpacing(m_gridView ? 8 : 2);
    m_browseList->setResizeMode(QListView::Adjust);
    m_browseList->setWordWrap(true);
    m_browseList->setStyleSheet(listStyle);
    m_browseList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_browseList->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Multi-select with Ctrl/Shift
    // Disable Qt's internal drag - we handle it ourselves with proper file URL mime data
    m_browseList->setDragEnabled(false);
    m_browseList->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_browseList->installEventFilter(this);  // For keyboard navigation
    m_browseList->viewport()->installEventFilter(this);  // For mouse events (drag)
    connect(m_browseList, &QListWidget::itemClicked,
            this, &MapBrowserWidget::onBrowseItemClicked);
    connect(m_browseList, &QListWidget::itemDoubleClicked,
            this, &MapBrowserWidget::onBrowseItemDoubleClicked);
    connect(m_browseList, &QListWidget::itemActivated,
            this, &MapBrowserWidget::onBrowseItemActivated);
    connect(m_browseList, &QWidget::customContextMenuRequested,
            this, &MapBrowserWidget::onBrowseContextMenu);
    m_mainLayout->addWidget(m_browseList, 1);

    setWidget(m_mainWidget);
    setMinimumWidth(200);

    // Initial favorites update
    updateFavoritesSection();
}

void MapBrowserWidget::setRecentFilesController(RecentFilesController* controller)
{
    m_recentFilesController = controller;
    refreshRecentFiles();
}

void MapBrowserWidget::refreshRecentFiles()
{
    updateRecentFilesSection();
    updateFavoritesSection();
}

void MapBrowserWidget::updateRecentFilesSection()
{
    m_recentList->clear();

    // Get recent files from settings
    QStringList recentFiles = SettingsManager::instance().loadRecentFiles();

    for (const QString& filePath : recentFiles) {
        if (QFile::exists(filePath)) {
            addFileToList(m_recentList, filePath);
        }
    }

    if (m_recentList->count() == 0) {
        QListWidgetItem* emptyItem = new QListWidgetItem("(No recent files)");
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setForeground(QColor(0x60, 0x60, 0x60));
        m_recentList->addItem(emptyItem);
    }
}

void MapBrowserWidget::updateFavoritesSection()
{
    m_favoritesList->clear();

    for (const QString& filePath : m_favorites) {
        if (QFile::exists(filePath)) {
            addFileToList(m_favoritesList, filePath, true);
        }
    }

    if (m_favoritesList->count() == 0) {
        QListWidgetItem* emptyItem = new QListWidgetItem("(No favorites - click ★ to add)");
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setForeground(QColor(0x60, 0x60, 0x60));
        m_favoritesList->addItem(emptyItem);
    }
}

void MapBrowserWidget::setBrowseDirectory(const QString& path)
{
    if (path.isEmpty() || !QDir(path).exists()) {
        return;
    }

    m_browseDirectory = path;
    m_pathEdit->setText(path);
    m_searchEdit->clear();  // Clear search when changing directory
    loadBrowseDirectory();

    // Save for next session
    QSettings settings;
    settings.setValue("mapBrowser/directory", path);
}

void MapBrowserWidget::loadBrowseDirectory()
{
    updateBrowseSection();
}

void MapBrowserWidget::scanDirectoryRecursive(const QString& path, int depth)
{
    if (depth > 5) return;  // Limit recursion depth to prevent runaway scanning

    QDir dir(path);
    if (!dir.exists()) return;

    // Get supported files
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.webp" << "*.bmp" << "*.gif";
    filters << "*.dd2vtt" << "*.uvtt" << "*.df2vtt";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
    QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // Add files from this directory
    for (const QFileInfo& fileInfo : files) {
        BrowseItem item;
        item.filePath = fileInfo.absoluteFilePath();
        item.fileName = fileInfo.fileName();
        item.isDirectory = false;
        item.fileSize = fileInfo.size();
        item.modifiedDate = fileInfo.lastModified();
        m_allBrowseItems.append(item);
    }

    // Recurse into subdirectories
    for (const QFileInfo& dirInfo : dirs) {
        // Add directory itself (only for current level, not recursive dirs)
        if (depth == 0) {
            BrowseItem item;
            item.filePath = dirInfo.absoluteFilePath();
            item.fileName = dirInfo.fileName();
            item.isDirectory = true;
            item.fileSize = 0;
            item.modifiedDate = dirInfo.lastModified();
            m_allBrowseItems.append(item);
        }

        // Scan subdirectory for files (these will be included in search results)
        scanDirectoryRecursive(dirInfo.absoluteFilePath(), depth + 1);
    }
}

void MapBrowserWidget::updateBrowseSection()
{
    m_browseList->clear();
    m_itemsByPath.clear();
    m_allBrowseItems.clear();

    if (m_browseDirectory.isEmpty()) {
        m_pathEdit->setText("");
        updateFileCount();
        return;
    }

    m_pathEdit->setText(m_browseDirectory);

    QDir dir(m_browseDirectory);
    if (!dir.exists()) {
        updateFileCount();
        return;
    }

    // Scan current directory and all subdirectories recursively
    scanDirectoryRecursive(m_browseDirectory, 0);

    // Sort the items
    sortBrowseItems();

    // Apply current filter (or show all if no filter)
    filterBrowseList(m_currentFilter);

    // Update file count
    updateFileCount();
}

void MapBrowserWidget::filterBrowseList(const QString& filter)
{
    m_currentFilter = filter;
    m_browseList->clear();
    m_itemsByPath.clear();

    QString filterLower = filter.toLower();

    for (const BrowseItem& browseItem : m_allBrowseItems) {
        // Apply filter
        if (!filterLower.isEmpty() && !browseItem.fileName.toLower().contains(filterLower)) {
            continue;
        }

        if (browseItem.isDirectory) {
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(browseItem.fileName);
            item->setData(Qt::UserRole, browseItem.filePath);
            item->setData(Qt::UserRole + 1, true);  // Mark as directory

            // Folder icon
            QPixmap folderIcon(96, 96);
            folderIcon.fill(QColor(0x2D, 0x2D, 0x2D));
            QPainter painter(&folderIcon);
            painter.setPen(QColor(0x4A, 0x90, 0xE2));
            painter.setBrush(QColor(0x3A, 0x70, 0xB0));
            painter.drawRoundedRect(15, 30, 66, 50, 4, 4);
            painter.drawRoundedRect(15, 25, 30, 15, 3, 3);
            painter.end();
            item->setIcon(QIcon(folderIcon));

            m_browseList->addItem(item);
        } else {
            addFileToList(m_browseList, browseItem.filePath);
        }
    }

    if (m_browseList->count() == 0) {
        QString emptyText = filterLower.isEmpty() ? "(No maps found)" : "(No matches)";
        QListWidgetItem* emptyItem = new QListWidgetItem(emptyText);
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setForeground(QColor(0x60, 0x60, 0x60));
        m_browseList->addItem(emptyItem);
    }
}

void MapBrowserWidget::addFileToList(QListWidget* list, const QString& filePath, bool showFavoriteStar)
{
    QFileInfo info(filePath);

    QString displayName = info.fileName();
    if (showFavoriteStar) {
        displayName = "★ " + displayName;
    }

    QListWidgetItem* item = new QListWidgetItem();
    item->setText(displayName);
    item->setData(Qt::UserRole, filePath);
    item->setData(Qt::UserRole + 1, false);  // Not a directory
    item->setToolTip(filePath);

    // Get thumbnail (may be placeholder if not cached)
    QPixmap thumbnail = ThumbnailCache::instance().getThumbnail(filePath);
    item->setIcon(QIcon(thumbnail));

    list->addItem(item);

    // Track for async thumbnail updates
    m_itemsByPath.insert(filePath, item);
}

bool MapBrowserWidget::isSupportedFile(const QString& filePath) const
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    static QStringList supported = {"png", "jpg", "jpeg", "webp", "bmp", "gif",
                                    "dd2vtt", "uvtt", "df2vtt"};
    return supported.contains(ext);
}

// Favorites management
void MapBrowserWidget::loadFavorites()
{
    QSettings settings;
    m_favorites = settings.value("mapBrowser/favorites", QStringList()).toStringList();

    // Remove any favorites that no longer exist
    QStringList validFavorites;
    for (const QString& path : m_favorites) {
        if (QFile::exists(path)) {
            validFavorites.append(path);
        }
    }
    if (validFavorites.size() != m_favorites.size()) {
        m_favorites = validFavorites;
        saveFavorites();
    }
}

void MapBrowserWidget::saveFavorites()
{
    QSettings settings;
    settings.setValue("mapBrowser/favorites", m_favorites);
}

bool MapBrowserWidget::isFavorite(const QString& filePath) const
{
    return m_favorites.contains(filePath);
}

void MapBrowserWidget::addFavorite(const QString& filePath)
{
    if (!m_favorites.contains(filePath) && QFile::exists(filePath)) {
        m_favorites.prepend(filePath);  // Add to front
        saveFavorites();
        updateFavoritesSection();
    }
}

void MapBrowserWidget::removeFavorite(const QString& filePath)
{
    if (m_favorites.removeOne(filePath)) {
        saveFavorites();
        updateFavoritesSection();
    }
}

void MapBrowserWidget::onRecentItemClicked(QListWidgetItem* item)
{
    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        emit mapSelected(filePath);
    }
}

void MapBrowserWidget::onFavoriteItemClicked(QListWidgetItem* item)
{
    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        emit mapSelected(filePath);
    }
}

void MapBrowserWidget::onBrowseItemClicked(QListWidgetItem* item)
{
    bool isDirectory = item->data(Qt::UserRole + 1).toBool();
    if (isDirectory) {
        // Navigate into directory
        QString dirPath = item->data(Qt::UserRole).toString();
        setBrowseDirectory(dirPath);
    }
    // For files, single-click just selects (use star button to favorite, double-click to load)
}

void MapBrowserWidget::onBrowseItemDoubleClicked(QListWidgetItem* item)
{
    bool isDirectory = item->data(Qt::UserRole + 1).toBool();
    QString filePath = item->data(Qt::UserRole).toString();

    if (isDirectory) {
        setBrowseDirectory(filePath);
    } else if (!filePath.isEmpty() && QFile::exists(filePath)) {
        emit mapSelected(filePath);
    }
}

void MapBrowserWidget::onBrowseButtonClicked()
{
    QString startDir = m_browseDirectory.isEmpty() ?
                       QDir::homePath() : m_browseDirectory;

    QString dir = QFileDialog::getExistingDirectory(this,
        "Select Maps Folder", startDir, QFileDialog::ShowDirsOnly);

    if (!dir.isEmpty()) {
        setBrowseDirectory(dir);
    }
}

void MapBrowserWidget::onParentDirectoryClicked()
{
    if (m_browseDirectory.isEmpty()) {
        return;
    }

    QDir dir(m_browseDirectory);
    if (dir.cdUp()) {
        setBrowseDirectory(dir.absolutePath());
    }
}

void MapBrowserWidget::onThumbnailReady(const QString& filePath, const QPixmap& thumbnail)
{
    // Update the item if it's in our list
    QListWidgetItem* item = m_itemsByPath.value(filePath, nullptr);
    if (item) {
        item->setIcon(QIcon(thumbnail));
    }
}

void MapBrowserWidget::onSearchTextChanged(const QString& text)
{
    filterBrowseList(text);
}

void MapBrowserWidget::onAddToFavoritesClicked()
{
    // Get selected item from browse list
    QListWidgetItem* selected = m_browseList->currentItem();
    if (!selected) {
        return;
    }

    bool isDirectory = selected->data(Qt::UserRole + 1).toBool();
    if (isDirectory) {
        return;  // Can't favorite directories
    }

    QString filePath = selected->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) {
        return;
    }

    if (isFavorite(filePath)) {
        // Already a favorite - remove it
        removeFavorite(filePath);
    } else {
        // Add to favorites
        addFavorite(filePath);
    }
}

void MapBrowserWidget::onRemoveFromFavoritesClicked()
{
    QListWidgetItem* selected = m_favoritesList->currentItem();
    if (!selected) {
        return;
    }

    QString filePath = selected->data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        removeFavorite(filePath);
    }
}

void MapBrowserWidget::onRefreshClicked()
{
    if (!m_browseDirectory.isEmpty()) {
        loadBrowseDirectory();
    }
    updateRecentFilesSection();
    updateFavoritesSection();
}

// ===== Context Menu Handlers =====

void MapBrowserWidget::onBrowseContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_browseList->itemAt(pos);
    if (!item) return;

    bool isDirectory = item->data(Qt::UserRole + 1).toBool();
    if (isDirectory) return;  // No context menu for directories

    QString filePath = item->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    showContextMenuForFile(filePath, m_browseList->viewport()->mapToGlobal(pos), false);
}

void MapBrowserWidget::onRecentContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_recentList->itemAt(pos);
    if (!item) return;

    QString filePath = item->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    showContextMenuForFile(filePath, m_recentList->viewport()->mapToGlobal(pos), false);
}

void MapBrowserWidget::onFavoritesContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_favoritesList->itemAt(pos);
    if (!item) return;

    QString filePath = item->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    showContextMenuForFile(filePath, m_favoritesList->viewport()->mapToGlobal(pos), true);
}

void MapBrowserWidget::showContextMenuForFile(const QString& filePath, const QPoint& globalPos, bool isInFavorites)
{
    QMenu menu(this);

    // Check if multiple files are selected (only for browse list, not favorites/recent)
    QList<QListWidgetItem*> selectedItems = m_browseList->selectedItems();
    int selectedFileCount = 0;
    for (QListWidgetItem* item : selectedItems) {
        if (!item->data(Qt::UserRole + 1).toBool()) {  // Not a directory
            selectedFileCount++;
        }
    }
    bool isMultiSelect = selectedFileCount > 1 && !isInFavorites;

    // Open / Load map (only for single selection)
    if (!isMultiSelect) {
        QAction* openAction = menu.addAction("Open Map");
        connect(openAction, &QAction::triggered, this, [this, filePath]() {
            if (QFile::exists(filePath)) {
                emit mapSelected(filePath);
            }
        });
        menu.addSeparator();
    }

    // Reveal in Finder/Explorer (only for single selection)
    if (!isMultiSelect) {
#ifdef Q_OS_MAC
        QAction* revealAction = menu.addAction("Reveal in Finder");
#else
        QAction* revealAction = menu.addAction("Show in Explorer");
#endif
        connect(revealAction, &QAction::triggered, this, [this, filePath]() {
            revealInFinder(filePath);
        });

        // Copy file path
        QAction* copyPathAction = menu.addAction("Copy File Path");
        connect(copyPathAction, &QAction::triggered, this, [this, filePath]() {
            copyFilePath(filePath);
        });
        menu.addSeparator();
    }

    // Add/Remove from favorites
    if (isMultiSelect) {
        QAction* addFavAction = menu.addAction(QString("★ Add %1 Files to Favorites").arg(selectedFileCount));
        connect(addFavAction, &QAction::triggered, this, &MapBrowserWidget::onBatchAddToFavorites);
    } else if (isInFavorites) {
        QAction* removeFavAction = menu.addAction("Remove from Favorites");
        connect(removeFavAction, &QAction::triggered, this, [this, filePath]() {
            removeFavorite(filePath);
        });
    } else {
        if (isFavorite(filePath)) {
            QAction* removeFavAction = menu.addAction("Remove from Favorites");
            connect(removeFavAction, &QAction::triggered, this, [this, filePath]() {
                removeFavorite(filePath);
            });
        } else {
            QAction* addFavAction = menu.addAction("★ Add to Favorites");
            connect(addFavAction, &QAction::triggered, this, [this, filePath]() {
                addFavorite(filePath);
            });
        }
    }

    menu.addSeparator();

    // File info (only for single selection)
    if (!isMultiSelect) {
        QAction* infoAction = menu.addAction("File Info...");
        connect(infoAction, &QAction::triggered, this, [this, filePath]() {
            showFileInfo(filePath);
        });
        menu.addSeparator();

        // Rename file (only for single selection)
        QAction* renameAction = menu.addAction("Rename...");
        connect(renameAction, &QAction::triggered, this, [this, filePath]() {
            renameFile(filePath);
        });
    }

    // Delete file(s)
    if (isMultiSelect) {
        QAction* deleteAction = menu.addAction(QString("Delete %1 Files").arg(selectedFileCount));
        connect(deleteAction, &QAction::triggered, this, &MapBrowserWidget::onBatchDelete);
    } else {
        QAction* deleteAction = menu.addAction("Delete");
        connect(deleteAction, &QAction::triggered, this, [this, filePath]() {
            deleteFile(filePath);
        });
    }

    menu.exec(globalPos);
}

void MapBrowserWidget::revealInFinder(const QString& filePath)
{
    QFileInfo info(filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, "File Not Found",
            QString("The file no longer exists:\n%1").arg(filePath));
        return;
    }

#ifdef Q_OS_MAC
    // macOS: open -R selects the file in Finder
    QProcess::startDetached("open", QStringList() << "-R" << filePath);
#elif defined(Q_OS_WIN)
    // Windows: explorer /select,path
    QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(filePath));
#else
    // Linux: xdg-open the containing directory
    QProcess::startDetached("xdg-open", QStringList() << info.absolutePath());
#endif
}

void MapBrowserWidget::copyFilePath(const QString& filePath)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(filePath);
    }
}

void MapBrowserWidget::renameFile(const QString& filePath)
{
    QFileInfo info(filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, "File Not Found",
            QString("The file no longer exists:\n%1").arg(filePath));
        return;
    }

    QString oldName = info.fileName();
    QString baseName = info.completeBaseName();
    QString extension = info.suffix();

    bool ok;
    QString newBaseName = QInputDialog::getText(this, "Rename File",
        "Enter new name:", QLineEdit::Normal, baseName, &ok);

    if (!ok || newBaseName.isEmpty()) {
        return;  // Cancelled
    }

    // Sanitize the name - remove invalid characters
    newBaseName = newBaseName.trimmed();
    newBaseName.remove(QRegularExpression("[/\\\\:*?\"<>|]"));

    if (newBaseName.isEmpty()) {
        QMessageBox::warning(this, "Invalid Name",
            "The name contains only invalid characters.");
        return;
    }

    QString newFileName = newBaseName + "." + extension;
    QString newFilePath = info.absolutePath() + "/" + newFileName;

    if (newFilePath == filePath) {
        return;  // No change
    }

    // Check if target already exists
    if (QFile::exists(newFilePath)) {
        QMessageBox::warning(this, "File Exists",
            QString("A file named '%1' already exists in this location.").arg(newFileName));
        return;
    }

    // Perform the rename
    QFile file(filePath);
    if (file.rename(newFilePath)) {
        // Update favorites if this file was a favorite
        int favIndex = m_favorites.indexOf(filePath);
        if (favIndex >= 0) {
            m_favorites[favIndex] = newFilePath;
            saveFavorites();
        }

        // Refresh the views
        loadBrowseDirectory();
        updateFavoritesSection();
        updateRecentFilesSection();
    } else {
        QMessageBox::critical(this, "Rename Failed",
            QString("Could not rename the file:\n%1").arg(file.errorString()));
    }
}

void MapBrowserWidget::deleteFile(const QString& filePath)
{
    QFileInfo info(filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, "File Not Found",
            QString("The file no longer exists:\n%1").arg(filePath));
        return;
    }

    // Confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::warning(this, "Delete File",
        QString("Are you sure you want to permanently delete this file?\n\n%1\n\nThis action cannot be undone.")
            .arg(info.fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Remove from favorites first
    removeFavorite(filePath);

    // Delete the file
    QFile file(filePath);
    if (file.remove()) {
        // Refresh the views
        loadBrowseDirectory();
        updateRecentFilesSection();
    } else {
        QMessageBox::critical(this, "Delete Failed",
            QString("Could not delete the file:\n%1").arg(file.errorString()));
    }
}

void MapBrowserWidget::showFileInfo(const QString& filePath)
{
    QFileInfo info(filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, "File Not Found",
            QString("The file no longer exists:\n%1").arg(filePath));
        return;
    }

    // Format file size
    qint64 size = info.size();
    QString sizeStr;
    if (size < 1024) {
        sizeStr = QString("%1 bytes").arg(size);
    } else if (size < 1024 * 1024) {
        sizeStr = QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    } else {
        sizeStr = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    }

    QString infoText = QString(
        "<b>File:</b> %1<br><br>"
        "<b>Location:</b> %2<br><br>"
        "<b>Size:</b> %3<br><br>"
        "<b>Type:</b> %4<br><br>"
        "<b>Created:</b> %5<br><br>"
        "<b>Modified:</b> %6"
    ).arg(info.fileName())
     .arg(info.absolutePath())
     .arg(sizeStr)
     .arg(info.suffix().toUpper())
     .arg(info.birthTime().toString("MMM d, yyyy h:mm AP"))
     .arg(info.lastModified().toString("MMM d, yyyy h:mm AP"));

    QMessageBox infoBox(this);
    infoBox.setWindowTitle("File Info");
    infoBox.setTextFormat(Qt::RichText);
    infoBox.setText(infoText);
    infoBox.setIcon(QMessageBox::Information);
    infoBox.exec();
}

// ===== Sorting =====

void MapBrowserWidget::sortBrowseItems()
{
    std::stable_sort(m_allBrowseItems.begin(), m_allBrowseItems.end(),
        [this](const BrowseItem& a, const BrowseItem& b) {
            // Directories always come first
            if (a.isDirectory != b.isDirectory) {
                return a.isDirectory;
            }

            int cmp = 0;
            switch (m_sortMode) {
                case SortMode::Name:
                    cmp = QString::compare(a.fileName, b.fileName, Qt::CaseInsensitive);
                    break;
                case SortMode::DateModified:
                    if (a.modifiedDate < b.modifiedDate) cmp = -1;
                    else if (a.modifiedDate > b.modifiedDate) cmp = 1;
                    else cmp = 0;
                    break;
                case SortMode::Size:
                    if (a.fileSize < b.fileSize) cmp = -1;
                    else if (a.fileSize > b.fileSize) cmp = 1;
                    else cmp = 0;
                    break;
            }

            return m_sortAscending ? (cmp < 0) : (cmp > 0);
        });
}

void MapBrowserWidget::onSortChanged(int index)
{
    // Index: 0=Name↑, 1=Name↓, 2=Date↑, 3=Date↓, 4=Size↑, 5=Size↓
    m_sortMode = static_cast<SortMode>(index / 2);
    m_sortAscending = (index % 2 == 0);

    // Save preference
    QSettings settings;
    settings.setValue("mapBrowser/sortMode", static_cast<int>(m_sortMode));
    settings.setValue("mapBrowser/sortAscending", m_sortAscending);

    // Re-sort and refresh
    sortBrowseItems();
    filterBrowseList(m_currentFilter);
}

// ===== View Mode =====

void MapBrowserWidget::onViewModeToggled()
{
    m_gridView = !m_gridView;
    applyViewMode();

    // Save preference
    QSettings settings;
    settings.setValue("mapBrowser/gridView", m_gridView);
}

void MapBrowserWidget::applyViewMode()
{
    if (m_gridView) {
        m_browseList->setViewMode(QListView::IconMode);
        m_browseList->setIconSize(QSize(96, 96));
        m_browseList->setGridSize(QSize(110, 130));
        m_browseList->setSpacing(8);
        m_viewModeButton->setText("☷");
        m_viewModeButton->setToolTip("Switch to list view");
    } else {
        m_browseList->setViewMode(QListView::ListMode);
        m_browseList->setIconSize(QSize(48, 48));
        m_browseList->setGridSize(QSize(-1, -1));
        m_browseList->setSpacing(2);
        m_viewModeButton->setText("☰");
        m_viewModeButton->setToolTip("Switch to grid view");
    }
}

// ===== Folder Bookmarks =====

void MapBrowserWidget::loadFolderBookmarks()
{
    QSettings settings;
    m_folderBookmarks = settings.value("mapBrowser/folderBookmarks", QStringList()).toStringList();

    // Remove any bookmarks that no longer exist
    QStringList validBookmarks;
    for (const QString& path : m_folderBookmarks) {
        if (QDir(path).exists()) {
            validBookmarks.append(path);
        }
    }
    if (validBookmarks.size() != m_folderBookmarks.size()) {
        m_folderBookmarks = validBookmarks;
        saveFolderBookmarks();
    }
}

void MapBrowserWidget::saveFolderBookmarks()
{
    QSettings settings;
    settings.setValue("mapBrowser/folderBookmarks", m_folderBookmarks);
}

void MapBrowserWidget::onBookmarkFolderClicked()
{
    if (m_browseDirectory.isEmpty()) {
        return;
    }

    if (m_folderBookmarks.contains(m_browseDirectory)) {
        // Already bookmarked, remove it
        m_folderBookmarks.removeOne(m_browseDirectory);
    } else {
        // Add bookmark
        m_folderBookmarks.prepend(m_browseDirectory);
    }
    saveFolderBookmarks();
}

void MapBrowserWidget::onBookmarksMenuRequested()
{
    QMenu menu(this);

    // Add current folder to bookmarks
    if (!m_browseDirectory.isEmpty()) {
        if (m_folderBookmarks.contains(m_browseDirectory)) {
            QAction* removeAction = menu.addAction("★ Remove Current Folder");
            connect(removeAction, &QAction::triggered, this, &MapBrowserWidget::onBookmarkFolderClicked);
        } else {
            QAction* addAction = menu.addAction("☆ Bookmark Current Folder");
            connect(addAction, &QAction::triggered, this, &MapBrowserWidget::onBookmarkFolderClicked);
        }
        menu.addSeparator();
    }

    // List existing bookmarks
    if (m_folderBookmarks.isEmpty()) {
        QAction* emptyAction = menu.addAction("(No bookmarks)");
        emptyAction->setEnabled(false);
    } else {
        for (const QString& path : m_folderBookmarks) {
            QFileInfo info(path);
            QString displayName = info.fileName();
            if (displayName.isEmpty()) {
                displayName = path;
            }

            QMenu* submenu = menu.addMenu("📁 " + displayName);
            submenu->setToolTipsVisible(true);
            submenu->setToolTip(path);

            QAction* goAction = submenu->addAction("Go to folder");
            connect(goAction, &QAction::triggered, this, [this, path]() {
                goToBookmarkedFolder(path);
            });

            QAction* removeAction = submenu->addAction("Remove bookmark");
            connect(removeAction, &QAction::triggered, this, [this, path]() {
                removeBookmarkedFolder(path);
            });
        }
    }

    menu.exec(m_bookmarkButton->mapToGlobal(QPoint(0, m_bookmarkButton->height())));
}

void MapBrowserWidget::goToBookmarkedFolder(const QString& path)
{
    if (QDir(path).exists()) {
        setBrowseDirectory(path);
    } else {
        QMessageBox::warning(this, "Folder Not Found",
            QString("The bookmarked folder no longer exists:\n%1").arg(path));
        removeBookmarkedFolder(path);
    }
}

void MapBrowserWidget::removeBookmarkedFolder(const QString& path)
{
    m_folderBookmarks.removeOne(path);
    saveFolderBookmarks();
}

// ===== Batch Operations =====

void MapBrowserWidget::onBatchAddToFavorites()
{
    QList<QListWidgetItem*> selected = m_browseList->selectedItems();
    int addedCount = 0;

    for (QListWidgetItem* item : selected) {
        bool isDirectory = item->data(Qt::UserRole + 1).toBool();
        if (isDirectory) continue;

        QString filePath = item->data(Qt::UserRole).toString();
        if (!filePath.isEmpty() && !isFavorite(filePath)) {
            m_favorites.append(filePath);
            addedCount++;
        }
    }

    if (addedCount > 0) {
        saveFavorites();
        updateFavoritesSection();
    }
}

void MapBrowserWidget::onBatchDelete()
{
    QList<QListWidgetItem*> selected = m_browseList->selectedItems();

    // Count files (not directories)
    int fileCount = 0;
    for (QListWidgetItem* item : selected) {
        bool isDirectory = item->data(Qt::UserRole + 1).toBool();
        if (!isDirectory) fileCount++;
    }

    if (fileCount == 0) return;

    // Confirmation
    QMessageBox::StandardButton reply = QMessageBox::warning(this, "Delete Files",
        QString("Are you sure you want to permanently delete %1 file(s)?\n\nThis action cannot be undone.")
            .arg(fileCount),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    int deletedCount = 0;
    for (QListWidgetItem* item : selected) {
        bool isDirectory = item->data(Qt::UserRole + 1).toBool();
        if (isDirectory) continue;

        QString filePath = item->data(Qt::UserRole).toString();
        if (!filePath.isEmpty()) {
            removeFavorite(filePath);
            if (QFile::remove(filePath)) {
                deletedCount++;
            }
        }
    }

    if (deletedCount > 0) {
        loadBrowseDirectory();
        updateRecentFilesSection();
    }
}

// ===== Keyboard Navigation =====

void MapBrowserWidget::onBrowseItemActivated(QListWidgetItem* item)
{
    // Triggered by Enter key
    if (!item) return;

    bool isDirectory = item->data(Qt::UserRole + 1).toBool();
    QString filePath = item->data(Qt::UserRole).toString();

    if (isDirectory) {
        setBrowseDirectory(filePath);
    } else if (!filePath.isEmpty() && QFile::exists(filePath)) {
        emit mapSelected(filePath);
    }
}

bool MapBrowserWidget::eventFilter(QObject* obj, QEvent* event)
{
    // Keyboard events come to the browse list widget
    if (obj == m_browseList) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            // Delete key - delete selected files
            if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace) {
                QList<QListWidgetItem*> selected = m_browseList->selectedItems();
                if (selected.size() == 1) {
                    QString filePath = selected.first()->data(Qt::UserRole).toString();
                    bool isDirectory = selected.first()->data(Qt::UserRole + 1).toBool();
                    if (!isDirectory && !filePath.isEmpty()) {
                        deleteFile(filePath);
                    }
                } else if (selected.size() > 1) {
                    onBatchDelete();
                }
                return true;
            }

            // F key - toggle favorite for selected
            if (keyEvent->key() == Qt::Key_F) {
                QList<QListWidgetItem*> selected = m_browseList->selectedItems();
                if (selected.size() == 1) {
                    onAddToFavoritesClicked();
                } else if (selected.size() > 1) {
                    onBatchAddToFavorites();
                }
                return true;
            }
        }
    }

    // Mouse events come to the viewport
    if (obj == m_browseList->viewport()) {
        // Track drag start position
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_dragStartPos = mouseEvent->pos();
            }
        }

        // Handle drag start manually to provide proper file URL mime data
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                // Check if we've moved enough to start a drag
                QPoint diff = mouseEvent->pos() - m_dragStartPos;
                if (diff.manhattanLength() > QApplication::startDragDistance()) {
                    QListWidgetItem* item = m_browseList->itemAt(m_dragStartPos);
                    if (item && !item->data(Qt::UserRole + 1).toBool()) {  // Not a directory
                        QString filePath = item->data(Qt::UserRole).toString();
                        if (!filePath.isEmpty()) {
                            QDrag* drag = new QDrag(this);
                            QMimeData* mimeData = new QMimeData();

                            // Add file URL
                            QList<QUrl> urls;
                            urls.append(QUrl::fromLocalFile(filePath));
                            mimeData->setUrls(urls);

                            // Also add as plain text for compatibility
                            mimeData->setText(filePath);

                            drag->setMimeData(mimeData);

                            // Create a small thumbnail for the drag icon
                            QPixmap thumbnail = ThumbnailCache::instance().getThumbnail(filePath);
                            if (!thumbnail.isNull()) {
                                drag->setPixmap(thumbnail.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            }

                            drag->exec(Qt::CopyAction);
                            return true;
                        }
                    }
                }
            }
        }
    }

    return QDockWidget::eventFilter(obj, event);
}

// ===== File Count =====

void MapBrowserWidget::updateFileCount()
{
    int fileCount = 0;
    int dirCount = 0;

    for (const BrowseItem& item : m_allBrowseItems) {
        if (item.isDirectory) {
            dirCount++;
        } else {
            fileCount++;
        }
    }

    QString countText;
    if (dirCount > 0 && fileCount > 0) {
        countText = QString("%1 files, %2 folders").arg(fileCount).arg(dirCount);
    } else if (fileCount > 0) {
        countText = QString("%1 files").arg(fileCount);
    } else if (dirCount > 0) {
        countText = QString("%1 folders").arg(dirCount);
    } else {
        countText = "Empty";
    }

    if (m_fileCountLabel) {
        m_fileCountLabel->setText(countText);
    }
}

// ===== Drag Support =====

void MapBrowserWidget::setupDragDrop()
{
    // Drag is enabled in setupUI - this is a placeholder for additional setup if needed
}
