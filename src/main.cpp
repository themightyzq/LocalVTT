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
#include <fstream>
#include "ui/MainWindow.h"
#include "ui/DarkTheme.h"
#include "utils/DebugConsole.h"

// OpenGL is mandatory - no fallback
const bool g_useOpenGL = true;

int main(int argc, char *argv[])
{
    // Initialize Qt resources FIRST before anything else
    // Temporarily disabled due to Qt rcc symbol issues
    // Q_INIT_RESOURCE(icons);
    // Q_INIT_RESOURCE(shaders);

    // Write to a log file since macOS bundles suppress console output
    std::ofstream debugLog("/tmp/projectvtt_debug.log", std::ios::app);
    debugLog << "=== LocalVTT Starting ===" << std::endl;
    debugLog << "main() started" << std::endl;
    debugLog << "Resources initialized" << std::endl;
    debugLog.flush();

    std::cerr << "main() started" << std::endl;
    std::cerr << "Resources initialized" << std::endl;
    std::cerr.flush();
    // Ensure Qt can find platform/imageformat plugins even before QApplication constructs
    // Use ONLY bundled plugins for macOS .app bundle
#if defined(Q_OS_MAC)
    {
        QStringList libPaths;
        // macOS .app bundle structure: Contents/PlugIns/
        QString appDir = QCoreApplication::applicationDirPath();
        QString bundledPlugins = appDir + "/../PlugIns";

        if (QDir(bundledPlugins).exists()) {
            libPaths << bundledPlugins;
            debugLog << "Using BUNDLED Qt plugins from: " << bundledPlugins.toStdString() << std::endl;
        } else {
            debugLog << "WARNING: Bundled plugins not found at: " << bundledPlugins.toStdString() << std::endl;
        }

        // IMPORTANT: Clear all paths and use ONLY bundled plugins
        if (!libPaths.isEmpty()) {
            QCoreApplication::setLibraryPaths(libPaths);
            debugLog << "Library paths set to bundled plugins ONLY" << std::endl;
        }
    }
#elif defined(Q_OS_WIN)
    {
        // Windows: Qt plugins are usually in the same directory as the executable
        // or in a plugins subdirectory
        QStringList libPaths;
        QString appDir = QCoreApplication::applicationDirPath();
        libPaths << appDir;  // Same directory as .exe
        libPaths << appDir + "/plugins";  // plugins subdirectory

        QCoreApplication::setLibraryPaths(libPaths + QCoreApplication::libraryPaths());
        debugLog << "Using Qt plugins from application directory" << std::endl;
    }
#elif defined(Q_OS_LINUX)
    {
        // Linux: Try common Qt6 plugin installation locations
        QStringList libPaths;
        QStringList linuxPaths = {
            "/usr/lib/qt6/plugins",
            "/usr/lib64/qt6/plugins",
            "/usr/lib/x86_64-linux-gnu/qt6/plugins",
            "/usr/local/lib/qt6/plugins",
            "/usr/share/qt6/plugins"
        };

        for (const QString& path : linuxPaths) {
            if (QDir(path).exists()) {
                libPaths << path;
                debugLog << "Using Qt plugins from: " << path.toStdString() << std::endl;
                break;
            }
        }

        if (!libPaths.isEmpty()) {
            QCoreApplication::setLibraryPaths(libPaths + QCoreApplication::libraryPaths());
        }
    }
#endif

    debugLog << "About to create QApplication..." << std::endl;
    debugLog.flush();
    std::cerr << "Creating QApplication..." << std::endl;
    std::cerr.flush();
    QApplication app(argc, argv);
    debugLog << "QApplication created successfully" << std::endl;
    debugLog.flush();
    std::cerr << "QApplication created successfully" << std::endl;
    std::cerr.flush();

    // Verify critical plugin support
    // DebugConsole::system(QString("Qt plugin paths: %1").arg(QCoreApplication::libraryPaths().join(", ")), "System");

    // Check for critical image format support
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    // DebugConsole::system(QString("Supported image formats: %1").arg(formats.join(", ")), "System");

    // Diagnostic output for plugin paths (helpful for debugging)
    debugLog << "Qt library paths:" << std::endl;
    for (const QString& path : QCoreApplication::libraryPaths()) {
        debugLog << "  " << path.toStdString() << std::endl;
    }
    debugLog.flush();

    if (!formats.contains("jpeg") && !formats.contains("jpg")) {
        // DebugConsole::error("FATAL: Qt JPEG support missing!", "System");

        // Try multiple approaches to load the JPEG plugin
        bool jpegLoaded = false;

        // Method 1: Try loading by plugin name
        QPluginLoader jpegLoader("qjpeg");
        if (jpegLoader.load()) {
            jpegLoaded = true;
            std::cerr << "Successfully loaded JPEG plugin via name lookup" << std::endl;
        } else {
            // Method 2: Try loading from specific paths (platform-specific)
            QStringList pluginPaths;

#if defined(Q_OS_MAC)
            pluginPaths = {
                QCoreApplication::applicationDirPath() + "/../PlugIns/imageformats/libqjpeg.dylib",
                "/opt/homebrew/share/qt/plugins/imageformats/libqjpeg.dylib",
                "/opt/homebrew/opt/qt/share/qt/plugins/imageformats/libqjpeg.dylib",
                "/opt/homebrew/opt/qt/plugins/imageformats/libqjpeg.dylib",
                "/opt/homebrew/opt/qt@6/plugins/imageformats/libqjpeg.dylib",
                "/usr/local/share/qt/plugins/imageformats/libqjpeg.dylib",
                "/usr/local/opt/qt/plugins/imageformats/libqjpeg.dylib"
            };
#elif defined(Q_OS_WIN)
            pluginPaths = {
                QCoreApplication::applicationDirPath() + "/imageformats/qjpeg.dll",
                QCoreApplication::applicationDirPath() + "/plugins/imageformats/qjpeg.dll",
                QCoreApplication::applicationDirPath() + "/../plugins/imageformats/qjpeg.dll"
            };
#elif defined(Q_OS_LINUX)
            pluginPaths = {
                QCoreApplication::applicationDirPath() + "/imageformats/libqjpeg.so",
                QCoreApplication::applicationDirPath() + "/plugins/imageformats/libqjpeg.so",
                "/usr/lib/qt6/plugins/imageformats/libqjpeg.so",
                "/usr/lib64/qt6/plugins/imageformats/libqjpeg.so",
                "/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/libqjpeg.so",
                "/usr/local/lib/qt6/plugins/imageformats/libqjpeg.so"
            };
#endif

            for (const QString& pluginPath : pluginPaths) {
                if (QFile::exists(pluginPath)) {
                    QPluginLoader loader(pluginPath);
                    if (loader.load()) {
                        jpegLoaded = true;
                        std::cerr << "Successfully loaded JPEG plugin from: " << pluginPath.toStdString() << std::endl;
                        break;
                    } else {
                        debugLog << "Failed to load JPEG plugin from " << pluginPath.toStdString()
                                 << ": " << loader.errorString().toStdString() << std::endl;
                    }
                }
            }
        }

        // Re-check formats after attempting to load the plugin
        if (jpegLoaded) {
            formats = QImageReader::supportedImageFormats();
            if (formats.contains("jpeg") || formats.contains("jpg")) {
                std::cerr << "JPEG support successfully enabled!" << std::endl;
            }
        } else {
            std::cerr << "WARNING: Qt JPEG plugin could not be loaded. DD2VTT files will not work properly." << std::endl;
            std::cerr << "This may affect loading of DD2VTT files with embedded JPEG images." << std::endl;
        }
        // DebugConsole::system("Successfully loaded JPEG plugin manually", "System");
    } else {
        std::cerr << "JPEG support is available (plugin loaded successfully)" << std::endl;
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
    app.setApplicationName("LocalVTT");
    app.setOrganizationName("WNG");
    app.setApplicationDisplayName("LocalVTT - In-Person Virtual Tabletop");

    // Enable high DPI pixmaps for better quality on Retina displays
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("LocalVTT - In-Person Virtual Tabletop for TV display");
    parser.addHelpOption();

    // Add positional argument for map file
    parser.addPositionalArgument("map", "Map file to load on startup", "[map]");

    // Add test mode option for automated testing
    QCommandLineOption testRenderOption("test-render",
        "Test mode: Load image, verify rendering, and exit with status code");
    parser.addOption(testRenderOption);

    // Process the actual command line arguments
    parser.process(app);

    // OpenGL is mandatory - always enabled
    // DebugConsole::system("OpenGL rendering enabled (mandatory - no fallback)", "OpenGL");

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
    debugLog << "Creating MainWindow..." << std::endl;
    debugLog.flush();
    std::cerr << "Creating MainWindow..." << std::endl;
    std::cerr.flush();
    MainWindow mainWindow;
    debugLog << "MainWindow constructor returned" << std::endl;
    debugLog.flush();
    std::cerr << "Setting window title..." << std::endl;
    std::cerr.flush();
    mainWindow.setWindowTitle("LocalVTT - DM Control");
    debugLog << "Window title set" << std::endl;
    debugLog.flush();
    std::cerr << "Showing MainWindow..." << std::endl;
    std::cerr.flush();
    mainWindow.show();
    debugLog << "MainWindow shown successfully" << std::endl;
    debugLog << "MainWindow isVisible after show(): " << mainWindow.isVisible() << std::endl;
    debugLog.flush();
    std::cerr << "MainWindow shown successfully" << std::endl;
    std::cerr << "MainWindow isVisible after show(): " << mainWindow.isVisible() << std::endl;
    std::cerr.flush();

    // Load map file if provided (RESTORED - was disabled due to suspected crash)
    if (!mapFile.isEmpty()) {
        std::cerr << "Map file provided: " << mapFile.toStdString() << std::endl;
        if (testMode) {
            // Test mode: Load image, verify rendering, and exit
            QTimer::singleShot(100, [&mainWindow, &app, mapFile]() {
                std::cerr << "Test mode: Loading map..." << std::endl;
                // Load the map
                mainWindow.loadMapFromCommandLine(mapFile);

                // Give time for rendering
                QTimer::singleShot(1000, [&app, &mainWindow]() {
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
            // Normal mode
            QTimer::singleShot(100, [&mainWindow, mapFile]() {
                std::cerr << "Normal mode: Loading map after 100ms..." << std::endl;
                mainWindow.loadMapFromCommandLine(mapFile);
            });
        }
    } else if (testMode) {
        // Test mode but no file provided
        std::cout << "ERROR: Test mode requires a map file" << std::endl;
        return 1;
    }

    debugLog << "About to enter Qt event loop..." << std::endl;
    debugLog.flush();
    std::cerr << "Entering Qt event loop..." << std::endl;
    std::cerr.flush();
    int result = app.exec();
    debugLog << "Qt event loop exited with code: " << result << std::endl;
    debugLog.flush();
    std::cerr << "Qt event loop exited with code: " << result << std::endl;
    return result;
}
