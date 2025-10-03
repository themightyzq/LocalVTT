# LocalVTT Build Instructions

## Prerequisites

### All Platforms
- **CMake** 3.16 or higher
- **Qt 6** (6.2 or higher recommended)
- **C++17** compatible compiler

### Platform-Specific Requirements

#### macOS
- Xcode Command Line Tools: `xcode-select --install`
- Qt via Homebrew: `brew install qt@6`
- CMake via Homebrew: `brew install cmake`

#### Linux
- Build essentials: `sudo apt-get install build-essential`
- Qt6 development packages: `sudo apt-get install qt6-base-dev qt6-svg-dev`
- CMake: `sudo apt-get install cmake`
- OpenGL development: `sudo apt-get install libgl1-mesa-dev`

#### Windows
- Visual Studio 2019 or newer (with C++ development tools)
- Qt6 from official installer
- CMake from official installer or via Chocolatey: `choco install cmake`

## Quick Build

### macOS/Linux
```bash
cd scripts
./build.sh
```

### Windows
```batch
cd scripts
build.bat
```

### macOS (with library fixing)
```bash
cd scripts
./build-and-run.command
```

## Build System Features

The build scripts provide:
- **Automatic CPU detection** for parallel builds
- **Platform detection** for correct executable paths
- **Error handling** with clear messages
- **Automatic launch** after successful build
- **Library path fixing** (macOS only) for Qt framework dependencies

## Troubleshooting

### Build Fails
1. Ensure all prerequisites are installed
2. Check CMake can find Qt: `cmake --version` and `qmake --version`
3. Clean build directory: `rm -rf build` (Unix) or `rmdir /s build` (Windows)

### Application Won't Launch (macOS)
- The `build-and-run.command` script automatically fixes Qt library paths
- If manual fixing needed: `codesign --deep --force -s - build/LocalVTT.app`

### Missing Qt Plugins
- Ensure Qt is properly installed for your platform
- Check `QT_PLUGIN_PATH` environment variable

## Manual Build

If scripts don't work, you can build manually:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)  # Linux
cmake --build . -j$(sysctl -n hw.ncpu)  # macOS
cmake --build . --config Release  # Windows
```

## Binary Locations

After successful build:
- **macOS**: `build/LocalVTT.app`
- **Linux**: `build/LocalVTT`
- **Windows**: `build/Release/LocalVTT.exe`