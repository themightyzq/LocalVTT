cmake_minimum_required(VERSION 3.16)
project(CritVTT VERSION 1.0.1 LANGUAGES CXX OBJCXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)  # Temporarily disabled due to Qt/zstd issue
set(CMAKE_AUTOUIC ON)

# Add build timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
add_compile_definitions(BUILD_TIMESTAMP="${BUILD_TIMESTAMP}")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")

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

# Set Qt6 path explicitly for macOS
if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew/Cellar/qt/6.9.3" CACHE STRING "Qt6 installation path")
endif()

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL Svg Multimedia)

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

# Also try to find plugins in the share/qt directory (common on Homebrew)
if(NOT QT_PLUGINS_DIR OR NOT EXISTS "${QT_PLUGINS_DIR}")
    set(QT_PLUGINS_DIR "/opt/homebrew/Cellar/qt/6.9.2/share/qt/plugins")
    if(EXISTS "${QT_PLUGINS_DIR}")
        message(STATUS "Using alternate Qt plugins directory: ${QT_PLUGINS_DIR}")
    endif()
endif()

# Set up project sources
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/PlayerWindow.cpp
    src/ui/ToolboxWidget.cpp
    src/ui/dialogs/SettingsDialog.cpp
    src/ui/dialogs/LightEditDialog.cpp
    src/ui/dialogs/SavePresetDialog.cpp
    src/ui/ToastNotification.cpp
    src/ui/LoadingOverlay.cpp
    src/ui/KeyboardShortcutsOverlay.cpp
    src/ui/DebugConsoleWidget.cpp
    src/ui/DarkTheme.cpp
    src/ui/widgets/ToolStatusWidget.cpp
    src/ui/widgets/ThumbnailCache.cpp
    src/ui/widgets/MapBrowserWidget.cpp
    src/ui/widgets/ColorPickerButton.cpp
    src/ui/AtmosphereToolboxWidget.cpp
    src/graphics/MapDisplay.cpp
    src/graphics/MouseInputManager.cpp
    src/graphics/SceneBuilder.cpp
    src/graphics/SceneAnimationDriver.cpp
    src/graphics/GridOverlay.cpp
    src/graphics/FogOfWar.cpp
    src/graphics/PingIndicator.cpp
    src/graphics/GMBeacon.cpp
    src/graphics/LightingOverlay.cpp
    src/graphics/WeatherEffect.cpp
    src/graphics/FogMistEffect.cpp
    src/graphics/LightningEffect.cpp
    src/graphics/PointLightSystem.cpp
    src/graphics/ZoomIndicator.cpp
    src/graphics/LoadingProgressWidget.cpp
    src/graphics/ToolOverlayWidget.cpp
    src/graphics/ImageCache.cpp
    src/audio/AmbientPlayer.cpp
    src/audio/MusicRemote.cpp
    src/audio/MusicRemote_mac.mm
    src/utils/ImageLoader.cpp
    src/utils/VTTLoader.cpp
    src/utils/MapSession.cpp
    src/utils/SettingsManager.cpp
    src/utils/CustomCursors.cpp
    src/utils/ErrorHandler.cpp
    src/utils/LogHandler.cpp
    src/utils/DebugConsole.cpp
    src/utils/ActionRegistry.cpp
    src/utils/MemoryManager.cpp
    src/utils/AnimationHelper.cpp
    src/utils/CustomPresetManager.cpp
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
    src/controllers/AtmosphereManager.cpp
    src/controllers/AtmosphereController.cpp
)

set(HEADERS
    src/ui/MainWindow.h
    src/ui/PlayerWindow.h
    src/ui/ToolboxWidget.h
    src/ui/dialogs/SettingsDialog.h
    src/ui/dialogs/LightEditDialog.h
    src/ui/dialogs/SavePresetDialog.h
    src/ui/ToastNotification.h
    src/ui/LoadingOverlay.h
    src/ui/KeyboardShortcutsOverlay.h
    src/ui/DebugConsoleWidget.h
    src/ui/DarkTheme.h
    src/ui/widgets/ToolStatusWidget.h
    src/ui/widgets/ThumbnailCache.h
    src/ui/widgets/MapBrowserWidget.h
    src/ui/widgets/ColorPickerButton.h
    src/ui/AtmosphereToolboxWidget.h
    src/graphics/MapDisplay.h
    src/graphics/MouseInputManager.h
    src/graphics/SceneBuilder.h
    src/graphics/SceneAnimationDriver.h
    src/graphics/GridOverlay.h
    src/graphics/FogOfWar.h
    src/graphics/PingIndicator.h
    src/graphics/GMBeacon.h
    src/graphics/LightingOverlay.h
    src/graphics/WeatherEffect.h
    src/graphics/FogMistEffect.h
    src/graphics/LightningEffect.h
    src/graphics/PointLight.h
    src/graphics/PointLightSystem.h
    src/graphics/ZoomIndicator.h
    src/graphics/LoadingProgressWidget.h
    src/graphics/ToolOverlayWidget.h
    src/audio/AmbientPlayer.h
    src/audio/MusicRemote.h
    src/utils/ImageLoader.h
    src/utils/VTTLoader.h
    src/utils/MapSession.h
    src/utils/SettingsManager.h
    src/utils/FogToolMode.h
    src/utils/CustomCursors.h
    src/utils/ErrorHandler.h
    src/utils/LogHandler.h
    src/utils/DebugConsole.h
    src/utils/ActionRegistry.h
    src/utils/ToolType.h
    src/utils/MemoryManager.h
    src/utils/AnimationHelper.h
    src/utils/CustomPresetManager.h
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
    src/controllers/AtmosphereState.h
    src/controllers/AtmospherePreset.h
    src/controllers/AtmosphereManager.h
    src/controllers/AtmosphereController.h
)

# Qt Resources for icons
qt6_add_resources(RESOURCES
    resources/icons.qrc
)

# Create executable
add_executable(CritVTT ${SOURCES} ${HEADERS} ${RESOURCES})

# Make headers discoverable for nested includes
target_include_directories(CritVTT PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/controllers
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${CMAKE_SOURCE_DIR}/src/audio
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(CritVTT PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Link Qt libraries
target_link_libraries(CritVTT PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Svg
    Qt6::Multimedia
)

# Platform-specific linking for macOS
if(APPLE)
    # Link only required frameworks, explicitly excluding AGL
    target_link_libraries(CritVTT PRIVATE
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
    set_target_properties(CritVTT PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "Crit VTT"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        INSTALL_RPATH "@loader_path/../Frameworks;/opt/homebrew/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    # Deploy Qt plugins for macOS bundle
    if(QT_PLUGINS_DIR)
        # Create plugin directories in bundle
        add_custom_command(TARGET CritVTT POST_BUILD
            COMMENT "Deploying Qt plugins..."
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/PlugIns/imageformats"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/PlugIns/platforms"
        )

        # Copy essential image format plugins (including SVG for icon support)
        foreach(plugin jpeg ico webp gif tiff svg)
            if(EXISTS "${QT_PLUGINS_DIR}/imageformats/libq${plugin}.dylib")
                add_custom_command(TARGET CritVTT POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_PLUGINS_DIR}/imageformats/libq${plugin}.dylib"
                        "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/PlugIns/imageformats/"
                )
            endif()
        endforeach()

        # Copy platform plugin (required for Qt apps on macOS)
        if(EXISTS "${QT_PLUGINS_DIR}/platforms/libqcocoa.dylib")
            add_custom_command(TARGET CritVTT POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_PLUGINS_DIR}/platforms/libqcocoa.dylib"
                    "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/PlugIns/platforms/"
            )
        endif()

        # Copy SVG icon engine plugin (required for SVG icon support)
        add_custom_command(TARGET CritVTT POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/PlugIns/iconengines"
        )
        if(EXISTS "${QT_PLUGINS_DIR}/iconengines/libqsvgicon.dylib")
            add_custom_command(TARGET CritVTT POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_PLUGINS_DIR}/iconengines/libqsvgicon.dylib"
                    "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/PlugIns/iconengines/"
            )
        endif()
    endif()

    # Create Resources directory and qt.conf to help Qt find plugins
    add_custom_command(TARGET CritVTT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E echo "[Paths]" > "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/Resources/qt.conf"
        COMMAND ${CMAKE_COMMAND} -E echo "Plugins = PlugIns" >> "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/Resources/qt.conf"
    )

    # DISABLED: macdeployqt causes duplicate Qt framework loading issues
    # For development, we'll use the system Qt from Homebrew
    # Only enable this for distribution builds
    set(USE_MACDEPLOYQT OFF CACHE BOOL "Use macdeployqt for deployment (causes issues in development)")

    if(USE_MACDEPLOYQT)
        find_program(MACDEPLOYQT_EXECUTABLE NAMES macdeployqt PATHS ${Qt6_DIR}/../../../bin /opt/homebrew/opt/qt@6/bin)
        if(MACDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET CritVTT POST_BUILD
                COMMENT "Running macdeployqt for complete deployment..."
                COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_BUNDLE_DIR:CritVTT>" -always-overwrite
                VERBATIM
                COMMENT "Re-signing app bundle to fix code signature..."
                COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:CritVTT>"
            )
        else()
            message(STATUS "macdeployqt not found - using manual plugin deployment")
        endif()
    else()
        message(STATUS "macdeployqt disabled for development builds (USE_MACDEPLOYQT=OFF)")
        # Just re-sign without bundling frameworks
        add_custom_command(TARGET CritVTT POST_BUILD
            COMMENT "Re-signing app bundle..."
            COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:CritVTT>"
        )
    endif()

elseif(WIN32)
    set_target_properties(CritVTT PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Deploy Qt plugins for Windows
    if(QT_PLUGINS_DIR)
        add_custom_command(TARGET CritVTT POST_BUILD
            COMMENT "Deploying Qt plugins for Windows..."
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:CritVTT>/imageformats"
        )

        foreach(plugin jpeg ico webp gif)
            if(EXISTS "${QT_PLUGINS_DIR}/imageformats/q${plugin}.dll")
                add_custom_command(TARGET CritVTT POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_PLUGINS_DIR}/imageformats/q${plugin}.dll"
                        "$<TARGET_FILE_DIR:CritVTT>/imageformats/"
                )
            endif()
        endforeach()
    endif()

    # Use windeployqt if available
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET CritVTT POST_BUILD
            COMMENT "Running windeployqt..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE} "$<TARGET_FILE:CritVTT>"
        )
    endif()
endif()

# Copy resources to correct location in app bundle
if(APPLE)
    # Resources should go in Contents/Resources, not MacOS directory
    add_custom_command(TARGET CritVTT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_BUNDLE_DIR:CritVTT>/Contents/Resources/resources"
        COMMENT "Copying resources to app bundle..."
    )
else()
    # For non-macOS, copy to executable directory
    add_custom_command(TARGET CritVTT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:CritVTT>/resources"
        COMMENT "Copying resources..."
    )
endif()

# Install rules
install(TARGETS CritVTT
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)