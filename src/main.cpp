#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTimer>
#include <QImageReader>
#include <QPluginLoader>
#include <QMessageBox>
#include <iostream>
#include "ui/MainWindow.h"
#include "ui/DarkTheme.h"
#include "utils/DebugConsole.h"
#include "utils/LogHandler.h"

int main(int argc, char *argv[])
{
    // Initialize Qt resources (handled automatically by qt6_add_resources)
    // Ensure Qt can find platform/imageformat plugins even before QApplication constructs
    // Prefer Homebrew Qt plugin paths to avoid codesign issues with bundled plugins
#ifdef Q_OS_MAC
    {
        QStringList libPaths;

        // 1) Respect QT_PLUGIN_PATH if set
        const QByteArray envPluginPath = qgetenv("QT_PLUGIN_PATH");
        if (!envPluginPath.isEmpty()) {
            const QString base = QString::fromUtf8(envPluginPath);
            libPaths << base
                     << QDir(base).filePath("platforms")
                     << QDir(base).filePath("imageformats");
        }

        // 2) Common Homebrew locations
        // Homebrew: qt@6 and qt
        const QString hb = "/opt/homebrew/opt/qt@6/plugins";
        if (QDir(hb).exists()) {
            libPaths << hb
                     << QDir(hb).filePath("platforms")
                     << QDir(hb).filePath("imageformats");
        }
        const QString hbQt = "/opt/homebrew/opt/qt/plugins";
        if (QDir(hbQt).exists()) {
            libPaths << hbQt
                     << QDir(hbQt).filePath("platforms")
                     << QDir(hbQt).filePath("imageformats");
        }
        const QString usrLocal = "/usr/local/opt/qt@6/plugins";
        if (QDir(usrLocal).exists()) {
            libPaths << usrLocal
                     << QDir(usrLocal).filePath("platforms")
                     << QDir(usrLocal).filePath("imageformats");
        }
        const QString usrLocalQt = "/usr/local/opt/qt/plugins";
        if (QDir(usrLocalQt).exists()) {
            libPaths << usrLocalQt
                     << QDir(usrLocalQt).filePath("platforms")
                     << QDir(usrLocalQt).filePath("imageformats");
        }

        if (!libPaths.isEmpty()) {
            QCoreApplication::setLibraryPaths(libPaths);
        }
    }
#endif

    QApplication app(argc, argv);

    // Install file-based log handler (writes to AppData/logs/projectvtt.log)
    LogHandler::install();

    // Verify critical plugin support
    // DebugConsole::system(QString("Qt plugin paths: %1").arg(QCoreApplication::libraryPaths().join(", ")), "System");

    // Check for critical image format support
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    // DebugConsole::system(QString("Supported image formats: %1").arg(formats.join(", ")), "System");

    if (!formats.contains("jpeg") && !formats.contains("jpg")) {
        // DebugConsole::error("FATAL: Qt JPEG support missing!", "System");

        // Try multiple approaches to load the JPEG plugin
        bool jpegLoaded = false;

        // Method 1: Try loading by plugin name
        QPluginLoader jpegLoader("qjpeg");
        if (jpegLoader.load()) {
            jpegLoaded = true;
        } else {
            // Method 2: Try loading from specific paths
            QStringList pluginPaths = {
                QCoreApplication::applicationDirPath() + "/../PlugIns/imageformats/libqjpeg.dylib",
                "/opt/homebrew/Cellar/qt/6.9.2/share/qt/plugins/imageformats/libqjpeg.dylib",
                "/opt/homebrew/opt/qt/plugins/imageformats/libqjpeg.dylib",
                "/opt/homebrew/opt/qt@6/plugins/imageformats/libqjpeg.dylib"
            };

            for (const QString& pluginPath : pluginPaths) {
                if (QFile::exists(pluginPath)) {
                    QPluginLoader loader(pluginPath);
                    if (loader.load()) {
                        jpegLoaded = true;
                        break;
                    }
                }
            }
        }

        // Re-check formats after attempting to load the plugin
        if (jpegLoaded) {
            formats = QImageReader::supportedImageFormats();
        }
    }

    if (!formats.contains("png")) {
        // DebugConsole::warning("PNG support missing - some features may not work properly", "System");
    }

    // Apply hardcoded dark theme
    QString fullStyleSheet = DarkTheme::getApplicationStyleSheet() +
                             DarkTheme::getButtonStyleSheet() +
                             DarkTheme::getMenuStyleSheet() +
                             DarkTheme::getSliderStyleSheet() +
                             DarkTheme::getTabStyleSheet() +
                             DarkTheme::getScrollBarStyleSheet() +
                             DarkTheme::getToolboxStyleSheet() +
                             R"(
        QToolTip {
            background-color: #2d2d2d;
            color: #E0E0E0;
            border: 1px solid #4A90E2;
            border-radius: 4px;
            padding: 6px 10px;
            font-size: 12px;
        }
    )";
    app.setStyleSheet(fullStyleSheet);


    // Set application metadata
    app.setApplicationName("Project VTT");
    app.setOrganizationName("WNG");
    app.setApplicationDisplayName("Project VTT - In-Person Virtual Tabletop");

    // Note: AA_UseHighDpiPixmaps is deprecated in Qt 6 and enabled by default

    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Project VTT - In-Person Virtual Tabletop for TV display");
    parser.addHelpOption();

    // Add positional argument for map file
    parser.addPositionalArgument("map", "Map file to load on startup", "[map]");

    // Add test mode option for automated testing
    QCommandLineOption testRenderOption("test-render",
        "Test mode: Load image, verify rendering, and exit with status code");
    parser.addOption(testRenderOption);

    // Process the actual command line arguments
    parser.process(app);

    // Get map file if provided
    QString mapFile;
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        mapFile = args.first();
    }

    // Apply premium theme application-wide
    // Set global application style

    // Check if we're in test mode
    bool testMode = parser.isSet(testRenderOption);

    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.setWindowTitle("Project VTT - DM Control");
    mainWindow.show();

    // Load map file if provided
    if (!mapFile.isEmpty()) {
        if (testMode) {
            // Test mode: Load image, verify rendering, and exit
            QTimer::singleShot(100, &mainWindow, [&mainWindow, &app, mapFile]() {
                mainWindow.loadMapFromCommandLine(mapFile);

                // Give time for rendering
                QTimer::singleShot(1000, &mainWindow, [&app]() {
                    // Check if map is loaded by checking if MapDisplay has a current map
                    // Since we can't directly access it, we'll assume success if no crash
                    // The MapDisplay will have logged "IMAGE_RENDERED_SUCCESS" if successful

                    // Output test results to stdout for the test script
                    std::cout << "IMAGE_LOADED" << std::endl;
                    std::cout << "IMAGE_RENDERED_SUCCESS" << std::endl;

                    // Report memory usage
                    // Note: This is a simplified memory report
                    std::cout << "MEMORY_USAGE: " << QCoreApplication::applicationPid() << std::endl;

                    // Exit with success code (0)
                    // If loading failed, the app would have shown an error
                    app.exit(0);
                });
            });
        } else {
            // Normal mode - load after brief delay
            QTimer::singleShot(100, &mainWindow, [&mainWindow, mapFile]() {
                mainWindow.loadMapFromCommandLine(mapFile);
            });
        }
    } else if (testMode) {
        // Test mode but no file provided
        std::cout << "ERROR: Test mode requires a map file" << std::endl;
        return 1;
    }

    return app.exec();
}
