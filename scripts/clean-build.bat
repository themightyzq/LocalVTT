@echo off
REM Clean build script for LocalVTT - removes all build artifacts

echo ================================================
echo LocalVTT - Clean Build
echo ================================================

set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..
cd /d "%PROJECT_DIR%"

REM Remove build directory
if exist build (
    echo Removing build directory...
    rmdir /s /q build
    echo ✅ Build directory removed
) else (
    echo Build directory doesn't exist - already clean
)

REM Remove any CMake cache files that might exist in project root
if exist CMakeCache.txt (
    echo Removing stray CMakeCache.txt...
    del /f CMakeCache.txt
)

if exist CMakeFiles (
    echo Removing stray CMakeFiles...
    rmdir /s /q CMakeFiles
)

echo.
echo ✅ Clean complete!
echo.
echo To rebuild, run:
echo   scripts\build.bat

pause