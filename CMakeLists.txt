cmake_minimum_required(VERSION 3.16)
project(LocalVTT VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)  # Temporarily disabled due to Qt/zstd issue
set(CMAKE_AUTOUIC ON)

# Add build timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
add_compile_definitions(BUILD_TIMESTAMP="${BUILD_TIMESTAMP}")

# Platform-specific configuration for macOS
if(APPLE)
    # Silence OpenGL deprecation warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
    # Set deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    # Prevent finding AGL framework
    set(CMAKE_FIND_FRAMEWORK LAST)
    # Explicitly disable AGL
    set(WrapOpenGL_AGL "" CACHE INTERNAL "")
endif()

# Set Qt6 path for macOS if not already provided by user
if(APPLE AND NOT DEFINED CMAKE_PREFIX_PATH)
    # Try common Homebrew locations as fallback only if user hasn't set CMAKE_PREFIX_PATH
    # Using /opt/homebrew/opt/qt and /usr/local/opt/qt which are symlinks to current versions
    if(EXISTS "/opt/homebrew/opt/qt")
        set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" CACHE STRING "Qt6 installation path")
        message(STATUS "Found Qt at /opt/homebrew/opt/qt (Apple Silicon)")
    elseif(EXISTS "/usr/local/opt/qt")
        set(CMAKE_PREFIX_PATH "/usr/local/opt/qt" CACHE STRING "Qt6 installation path")
        message(STATUS "Found Qt at /usr/local/opt/qt (Intel Mac)")
    elseif(EXISTS "/opt/homebrew/opt/qt@6")
        set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@6" CACHE STRING "Qt6 installation path")
        message(STATUS "Found Qt at /opt/homebrew/opt/qt@6 (versioned formula)")
    elseif(EXISTS "/usr/local/opt/qt@6")
        set(CMAKE_PREFIX_PATH "/usr/local/opt/qt@6" CACHE STRING "Qt6 installation path")
        message(STATUS "Found Qt at /usr/local/opt/qt@6 (versioned formula)")
    else()
        message(STATUS "Qt path not found in standard Homebrew locations, CMake will search system paths")
    endif()
endif()

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL OpenGLWidgets Svg)

# Get Qt plugin directory for deployment
if(Qt6_FOUND)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    if(QT_QMAKE_EXECUTABLE)
        execute_process(
            COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE QT_PLUGINS_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(STATUS "Qt plugins directory: ${QT_PLUGINS_DIR}")
    endif()
endif()

# Try to find plugins directory using platform-appropriate fallbacks
if(NOT QT_PLUGINS_DIR OR NOT EXISTS "${QT_PLUGINS_DIR}")
    if(APPLE)
        # Try common Homebrew plugin locations (version-agnostic)
        foreach(plugin_path
            "/opt/homebrew/share/qt/plugins"
            "/opt/homebrew/opt/qt/share/qt/plugins"
            "/usr/local/share/qt/plugins"
            "/usr/local/opt/qt/share/qt/plugins"
            "/opt/homebrew/opt/qt@6/share/qt/plugins"
            "/usr/local/opt/qt@6/share/qt/plugins")
            if(EXISTS "${plugin_path}")
                set(QT_PLUGINS_DIR "${plugin_path}")
                message(STATUS "Found Qt plugins directory: ${QT_PLUGINS_DIR}")
                break()
            endif()
        endforeach()
    elseif(WIN32)
        # Windows: Try to find plugins relative to Qt installation
        get_target_property(QT_CORE_LOCATION Qt6::Core LOCATION)
        if(QT_CORE_LOCATION)
            get_filename_component(QT_BIN_DIR "${QT_CORE_LOCATION}" DIRECTORY)
            set(QT_PLUGINS_DIR "${QT_BIN_DIR}/../plugins")
            if(EXISTS "${QT_PLUGINS_DIR}")
                message(STATUS "Found Qt plugins directory: ${QT_PLUGINS_DIR}")
            endif()
        endif()
    elseif(UNIX AND NOT APPLE)
        # Linux: Try common installation paths
        foreach(plugin_path
            "/usr/lib/qt6/plugins"
            "/usr/lib64/qt6/plugins"
            "/usr/lib/x86_64-linux-gnu/qt6/plugins"
            "/usr/local/lib/qt6/plugins"
            "${CMAKE_PREFIX_PATH}/plugins")
            if(EXISTS "${plugin_path}")
                set(QT_PLUGINS_DIR "${plugin_path}")
                message(STATUS "Found Qt plugins directory: ${QT_PLUGINS_DIR}")
                break()
            endif()
        endforeach()
    endif()

    if(NOT QT_PLUGINS_DIR OR NOT EXISTS "${QT_PLUGINS_DIR}")
        message(WARNING "Qt plugins directory not found. Plugin deployment may not work correctly.")
    endif()
endif()

# Set up project sources
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/PlayerWindow.cpp
    src/ui/ToolboxWidget.cpp
    src/ui/dialogs/SettingsDialog.cpp
    src/ui/ToastNotification.cpp
    src/ui/LoadingOverlay.cpp
    src/ui/DebugConsoleWidget.cpp
    src/ui/DarkTheme.cpp
    src/ui/widgets/ToolStatusWidget.cpp
    src/graphics/MapDisplay.cpp
    src/graphics/MouseInputManager.cpp
    src/graphics/ZoomManager.cpp
    src/graphics/SceneManager.cpp
    src/graphics/MapImageManager.cpp
    src/graphics/GridOverlay.cpp
    src/graphics/FogOfWar.cpp
    src/graphics/WallSystem.cpp
    src/graphics/PortalSystem.cpp
    src/graphics/Portal.cpp
    src/graphics/PingIndicator.cpp
    src/graphics/GMBeacon.cpp
    src/graphics/LightingOverlay.cpp
    src/graphics/ZoomIndicator.cpp
    src/graphics/LoadingProgressWidget.cpp
    src/graphics/ToolOverlayWidget.cpp
    src/graphics/ImageCache.cpp
    src/opengl/OpenGLMapDisplay.cpp
    src/opengl/ShaderManager.cpp
    src/opengl/PostProcessingSystem.cpp
    src/utils/ImageLoader.cpp
    src/utils/VTTLoader.cpp
    src/utils/MapSession.cpp
    src/utils/SettingsManager.cpp
    src/utils/CustomCursors.cpp
    src/utils/ErrorHandler.cpp
    src/utils/DebugConsole.cpp
    src/utils/ActionRegistry.cpp
    src/utils/MemoryManager.cpp
    src/utils/AnimationHelper.cpp
    src/utils/SecureWindowRegistry.cpp
    src/controllers/RecentFilesController.cpp
    src/controllers/TabsController.cpp
    src/controllers/FogAutosaveController.cpp
    src/controllers/FogToolsController.cpp
    src/controllers/MenuManager.cpp
    src/controllers/FileOperationsManager.cpp
    src/controllers/ToolManager.cpp
    src/controllers/GridController.cpp
    src/controllers/LightingController.cpp
    src/controllers/ViewZoomController.cpp
    src/controllers/PlayerWindowController.cpp
)

set(HEADERS
    src/ui/MainWindow.h
    src/ui/PlayerWindow.h
    src/ui/ToolboxWidget.h
    src/ui/dialogs/SettingsDialog.h
    src/ui/ToastNotification.h
    src/ui/LoadingOverlay.h
    src/ui/DebugConsoleWidget.h
    src/ui/DarkTheme.h
    src/ui/widgets/ToolStatusWidget.h
    src/graphics/MapDisplay.h
    src/graphics/MouseInputManager.h
    src/graphics/ZoomManager.h
    src/graphics/SceneManager.h
    src/graphics/MapImageManager.h
    src/graphics/GridOverlay.h
    src/graphics/FogOfWar.h
    src/graphics/WallSystem.h
    src/graphics/PortalSystem.h
    src/graphics/Portal.h
    src/graphics/PingIndicator.h
    src/graphics/GMBeacon.h
    src/graphics/LightingOverlay.h
    src/graphics/ZoomIndicator.h
    src/graphics/LoadingProgressWidget.h
    src/graphics/ToolOverlayWidget.h
    src/opengl/OpenGLMapDisplay.h
    src/opengl/ShaderManager.h
    src/opengl/PostProcessingSystem.h
    src/utils/ImageLoader.h
    src/utils/VTTLoader.h
    src/utils/MapSession.h
    src/utils/SettingsManager.h
    src/utils/FogToolMode.h
    src/utils/CustomCursors.h
    src/utils/ErrorHandler.h
    src/utils/DebugConsole.h
    src/utils/ActionRegistry.h
    src/utils/ToolType.h
    src/utils/MemoryManager.h
    src/utils/AnimationHelper.h
    src/controllers/RecentFilesController.h
    src/controllers/TabsController.h
    src/controllers/FogAutosaveController.h
    src/controllers/FogToolsController.h
    src/controllers/MenuManager.h
    src/controllers/FileOperationsManager.h
    src/controllers/ToolManager.h
    src/controllers/GridController.h
    src/controllers/LightingController.h
    src/controllers/ViewZoomController.h
    src/controllers/PlayerWindowController.h
)

# Resources temporarily disabled due to Qt rcc symbol issues
# qt6_add_resources(RESOURCES
#     resources/icons.qrc
#     resources/shaders.qrc
# )

# Create executable
add_executable(LocalVTT ${SOURCES} ${HEADERS})

# Make headers discoverable for nested includes
target_include_directories(LocalVTT PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/controllers
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${CMAKE_SOURCE_DIR}/src/opengl
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Link Qt libraries
target_link_libraries(LocalVTT PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Svg
)

# Platform-specific linking for macOS
if(APPLE)
    # Link only required frameworks, explicitly excluding AGL
    target_link_libraries(LocalVTT PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework AppKit"
        "-framework Metal"
        "-framework ImageIO"
        "-framework CoreGraphics"
        "-framework Foundation"
    )
endif()

# AGL framework is deprecated and not available in recent macOS
# Qt6::OpenGL may try to link it, so we filter it out in the build script

# Set executable properties
if(APPLE)
    set_target_properties(LocalVTT PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "LocalVTT"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        # Disable RPATH - we'll use @executable_path directly in the build script
        BUILD_WITH_INSTALL_RPATH FALSE
        SKIP_BUILD_RPATH TRUE
        INSTALL_RPATH ""
    )

    # Deploy Qt plugins for macOS bundle
    if(QT_PLUGINS_DIR)
        # Create plugin directories in bundle
        add_custom_command(TARGET LocalVTT POST_BUILD
            COMMENT "Deploying Qt plugins..."
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/PlugIns/imageformats"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/PlugIns/platforms"
        )

        # Copy essential image format plugins (including SVG for icon support)
        foreach(plugin jpeg ico webp gif tiff svg)
            if(EXISTS "${QT_PLUGINS_DIR}/imageformats/libq${plugin}.dylib")
                add_custom_command(TARGET LocalVTT POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_PLUGINS_DIR}/imageformats/libq${plugin}.dylib"
                        "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/PlugIns/imageformats/"
                )
            endif()
        endforeach()

        # Copy platform plugin (required for Qt apps on macOS)
        if(EXISTS "${QT_PLUGINS_DIR}/platforms/libqcocoa.dylib")
            add_custom_command(TARGET LocalVTT POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_PLUGINS_DIR}/platforms/libqcocoa.dylib"
                    "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/PlugIns/platforms/"
            )
        endif()

        # Copy SVG icon engine plugin (required for SVG icon support)
        add_custom_command(TARGET LocalVTT POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/PlugIns/iconengines"
        )
        if(EXISTS "${QT_PLUGINS_DIR}/iconengines/libqsvgicon.dylib")
            add_custom_command(TARGET LocalVTT POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_PLUGINS_DIR}/iconengines/libqsvgicon.dylib"
                    "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/PlugIns/iconengines/"
            )
        endif()
    endif()

    # Create Resources directory and qt.conf to help Qt find plugins
    add_custom_command(TARGET LocalVTT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E echo "[Paths]" > "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/Resources/qt.conf"
        COMMAND ${CMAKE_COMMAND} -E echo "Plugins = PlugIns" >> "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/Resources/qt.conf"
    )

    # DISABLED: macdeployqt causes duplicate Qt framework loading issues
    # For development, we'll use the system Qt from Homebrew
    # Only enable this for distribution builds
    set(USE_MACDEPLOYQT OFF CACHE BOOL "Use macdeployqt for deployment (causes issues in development)")
    set(SKIP_CODESIGN OFF CACHE BOOL "Skip code signing during CMake build (for CI builds that sign separately)")

    if(USE_MACDEPLOYQT)
        find_program(MACDEPLOYQT_EXECUTABLE NAMES macdeployqt
            PATHS
                ${Qt6_DIR}/../../../bin
                /opt/homebrew/opt/qt/bin
                /opt/homebrew/opt/qt@6/bin
                /usr/local/opt/qt/bin
                /usr/local/opt/qt@6/bin
                ${CMAKE_PREFIX_PATH}/bin)
        if(MACDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET LocalVTT POST_BUILD
                COMMENT "Running macdeployqt for complete deployment..."
                COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_BUNDLE_DIR:LocalVTT>" -always-overwrite
                VERBATIM
                COMMENT "Re-signing app bundle to fix code signature..."
                COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:LocalVTT>"
            )
        else()
            message(STATUS "macdeployqt not found - using manual plugin deployment")
        endif()
    else()
        message(STATUS "macdeployqt disabled for development builds (USE_MACDEPLOYQT=OFF)")
        # Only re-sign if not skipping (CI builds handle signing separately)
        if(NOT SKIP_CODESIGN)
            add_custom_command(TARGET LocalVTT POST_BUILD
                COMMENT "Re-signing app bundle..."
                COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:LocalVTT>"
            )
        else()
            message(STATUS "Code signing skipped (SKIP_CODESIGN=ON) - CI build will sign after Qt bundling")
        endif()
    endif()

elseif(WIN32)
    set_target_properties(LocalVTT PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Deploy Qt plugins for Windows
    if(QT_PLUGINS_DIR)
        add_custom_command(TARGET LocalVTT POST_BUILD
            COMMENT "Deploying Qt plugins for Windows..."
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:LocalVTT>/imageformats"
        )

        foreach(plugin jpeg ico webp gif)
            if(EXISTS "${QT_PLUGINS_DIR}/imageformats/q${plugin}.dll")
                add_custom_command(TARGET LocalVTT POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_PLUGINS_DIR}/imageformats/q${plugin}.dll"
                        "$<TARGET_FILE_DIR:LocalVTT>/imageformats/"
                )
            endif()
        endforeach()
    endif()

    # Use windeployqt if available
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET LocalVTT POST_BUILD
            COMMENT "Running windeployqt..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE} "$<TARGET_FILE:LocalVTT>"
        )
    endif()
endif()

# Copy resources to correct location in app bundle
if(APPLE)
    # Resources should go in Contents/Resources, not MacOS directory
    add_custom_command(TARGET LocalVTT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_BUNDLE_DIR:LocalVTT>/Contents/Resources/resources"
        COMMENT "Copying resources to app bundle..."
    )
else()
    # For non-macOS, copy to executable directory
    add_custom_command(TARGET LocalVTT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:LocalVTT>/resources"
        COMMENT "Copying resources..."
    )
endif()

# Install rules
install(TARGETS LocalVTT
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)