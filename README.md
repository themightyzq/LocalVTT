# LocalVTT - Living Maps for In-Person Tabletop Gaming

![Build Status](https://github.com/yourusername/LocalVTT/workflows/Multi-Platform%20Build/badge.svg)
![License](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)
![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey)

Transform your TV into a living game map for **in-person** tabletop RPGs. LocalVTT displays maps with fog of war control, letting you reveal areas as players explore - all understandable at a glance with zero learning curve.

**Design Philosophy:** Everything visual. No hidden shortcuts. No modifier keys. Just click buttons and watch your world come alive.

---

## Quick Start (30 seconds)

1. Connect TV as second display
2. Launch LocalVTT
3. Drag map image onto window
4. Click **[Fog Mode]** → Map goes black on TV
5. Click **[Reveal Brush]** → Paint to reveal areas
6. Double-click anywhere → Beacon appears for players

**That's it.** No tutorial needed.

---

## Download Pre-Built Binaries

Don't want to build from source? Download the latest release:

**[Download from GitHub Releases](https://github.com/yourusername/LocalVTT/releases/latest)**

- **macOS**: LocalVTT.app (macOS 10.15+, Universal Binary)
- **Windows**: LocalVTT.exe (Windows 10+, x64)
- **Linux**: LocalVTT (Ubuntu 22.04+, x64)

Or get the latest development builds from [GitHub Actions](https://github.com/yourusername/LocalVTT/actions).

---

## Build from Source

### Prerequisites

- **Qt 6.5 or later**
- **CMake 3.16 or later**
- **C++17 compatible compiler**
- **OpenGL 3.3 or later**

### macOS Installation

1. Install Qt 6 via Homebrew:
   ```bash
   brew install qt
   ```

2. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/LocalVTT.git
   cd LocalVTT
   ```

3. Build and run:
   ```bash
   ./scripts/build-and-run.command
   ```

The application will build and launch automatically.

### Linux Installation

1. Install Qt 6 development packages:
   ```bash
   # Ubuntu/Debian
   sudo apt install qt6-base-dev qt6-svg-dev cmake build-essential

   # Fedora
   sudo dnf install qt6-qtbase-devel qt6-qtsvg-devel cmake gcc-c++

   # Arch
   sudo pacman -S qt6-base qt6-svg cmake gcc
   ```

2. Clone and build:
   ```bash
   git clone https://github.com/yourusername/LocalVTT.git
   cd LocalVTT
   chmod +x scripts/build.sh
   ./scripts/build.sh
   ```

### Windows Installation

1. Install Qt 6 and CMake:
   - Download Qt 6: https://www.qt.io/download-qt-installer
   - Download CMake: https://cmake.org/download/
   - Install Visual Studio 2019 or later (Community Edition is free)

2. Clone and build:
   ```cmd
   git clone https://github.com/yourusername/LocalVTT.git
   cd LocalVTT
   scripts\build.bat
   ```

---

## How to Use

### Loading Maps

**Supported formats:**
- Images: PNG, JPG, WebP, BMP, GIF
- VTT files: DD2VTT (DungeonDraft), UVTT (Universal VTT), DF2VTT (FoundryVTT)

**To load:**
- Click **[Load Map]** button
- Or drag & drop image onto window
- Or use **File → Open Map** (Ctrl+O)

### Fog of War

1. **Enable Fog Mode**: Click **[Fog Mode]** button
   - Map goes black on TV
   - Fog tools become enabled
   - DM can still see full map

2. **Reveal Areas**:
   - **[Reveal Brush]**: Click/drag to paint circular areas
   - **[Reveal Rectangle]**: Drag to reveal rectangular rooms
   - Adjust brush size with spinner (10-400px)

3. **Reset Fog**: Click **[Reset Fog]** (requires confirmation)

### Player Display

1. Click **[Player View]** to open TV window
2. Position window on second display (TV)
3. Player window automatically fits to screen
4. Shows only revealed areas (fog is 100% opaque)

### Beacons

Double-click anywhere on the map to place a beacon:
- Pulsing animated circle
- Visible on both DM and player displays
- Draws player attention to specific locations
- Fades out automatically

### Grid Overlay

1. Click **[Grid]** to toggle grid overlay
2. Adjust size with spinner (20-200px)
3. Works independently of fog mode

### Zoom Controls

- **[Zoom Out]** / **[Zoom In]**: Incremental zoom (20% steps)
- **[Fit]**: Fit map to window
- **Zoom Spinner**: Manual zoom entry (10-500%)
- Middle-click + drag: Pan around map

---

## Controls Reference

### Mouse Actions

| Action | Behavior |
|--------|----------|
| **Single click** | Reveal fog / Select button |
| **Drag** | Paint brush / Draw rectangle |
| **Double-click** | Place beacon (works everywhere) |
| **Middle-click drag** | Pan map |
| **Scroll wheel** | Zoom in/out |

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `F` | Toggle Fog Mode |
| `G` | Toggle Grid |
| `Escape` | Clear selection |
| `Ctrl+Z` | Undo fog change |
| `Ctrl+Y` | Redo fog change |
| `Ctrl+O` | Open map |
| `+` / `-` | Zoom in/out |
| `0` | Fit to window |
| `P` | Toggle player window |

---

## Toolbar Overview

```
[Load Map] | [Fog Mode] [Reveal Brush] [Reveal Rectangle] [Reset Fog] | [Zoom Out] [Zoom In] [Fit] | [Grid] | [Player View] | Brush: [200px▴▾] Grid: [150px▴▾] Zoom: [100%▴▾]
```

**Button Behavior:**
- **Load Map**: Opens file picker
- **Fog Mode**: Master switch (ON = fog tools enabled)
- **Reveal Brush**: Circle brush for organic exploration
- **Reveal Rectangle**: Drag to reveal rooms
- **Reset Fog**: Clear all fog (requires confirmation)
- **Zoom Out/In/Fit**: Precise zoom control
- **Grid**: Show/hide grid overlay
- **Player View**: Open/close TV window
- **Spinners**: Compact controls for brush/grid/zoom

**Glanceable Design:** All controls always visible. Disabled buttons are grayed out to show when features are inactive.

---

## DM Workflow Example

### Session Prep
1. Load dungeon map
2. Map visible on DM screen + TV

### Players Arrive
1. Click **[Fog Mode]** → TV goes black
2. **[Reveal Brush]** auto-selected

### During Play
- Players enter room → Click/drag to reveal
- "Look at the statue!" → Double-click statue → Beacon appears
- Reveal entire corridor → **[Reveal Rectangle]** → Drag
- Made a mistake → **[Reset Fog]** → Start over

### Session End
- Toggle **[Fog Mode]** OFF → Show full map
- "Here's what you missed!"

---

## System Requirements

### Minimum
- 4GB RAM
- OpenGL 3.3 graphics
- 1920x1080 display
- macOS 10.15+ or Linux

### Recommended
- 8GB RAM
- Dedicated GPU
- 4K TV for player display
- Dual monitor setup

---

## Features

- ✅ **Fog of War** - Reveal/hide areas as players explore
- ✅ **Dual Display** - DM control + TV player view
- ✅ **Beacons** - Direct player attention with double-click
- ✅ **Grid Overlay** - Optional square grid (adjustable size)
- ✅ **Multi-Tab** - Up to 10 maps, instant switching
- ✅ **VTT Support** - DD2VTT, UVTT, DF2VTT formats
- ✅ **Undo/Redo** - Fog history for mistakes
- ✅ **Recent Files** - Quick access to last 10 maps
- ✅ **Dark Theme** - Clean, professional interface
- ✅ **Glanceable UI** - Self-explanatory toolbar

---

## What LocalVTT Does NOT Do

LocalVTT is laser-focused on **maps and fog**. It intentionally does NOT include:

❌ **Online/multiplayer** - In-person gaming only
❌ **Dice rolling** - Use physical dice
❌ **Character sheets** - Use D&D Beyond/paper
❌ **Combat tracking** - Theater of mind
❌ **Measurement tools** - Handle at table
❌ **Weather/lighting** - Future feature
❌ **Drawing/notes** - Keep it simple

**Why?** Every feature removed is one less thing that can break. We ship **quality over quantity**.

---

## Troubleshooting

### Build Issues

**Problem**: CMake can't find Qt6
**Solution**: Set CMAKE_PREFIX_PATH to your Qt installation
```bash
export CMAKE_PREFIX_PATH=/opt/homebrew/Cellar/qt/6.9.2
cmake ..
```

**Problem**: OpenGL deprecation warnings on macOS
**Solution**: These are harmless - OpenGL is still supported through compatibility

### Runtime Issues

**Problem**: Map doesn't load
**Solution**:
- Check file format (PNG, JPG, WebP supported)
- Try smaller file (<100MB recommended)
- Check console for error messages

**Problem**: Player window shows fog incorrectly
**Solution**: Close and reopen player window (P key twice)

**Problem**: Fog reveals are slow
**Solution**: Reduce brush size or use smaller map images

---

## License

**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**

LocalVTT is free for **personal, non-commercial use**. You may:
- ✅ Use it for your tabletop gaming sessions
- ✅ Modify it for your own needs
- ✅ Share your modifications with others (under the same license)

You may NOT:
- ❌ Sell this software or charge money for it
- ❌ Use it for commercial purposes (paid events, commercial streaming, etc.)
- ❌ Remove attribution to the original creator

See [LICENSE](LICENSE) for full legal text or visit https://creativecommons.org/licenses/by-nc-sa/4.0/

---

## Credits

- Built with Qt Framework
- OpenGL rendering
- Community feedback and testing

---

**Your table. Your dice. Your TV becomes another world.**

*LocalVTT - Atmospheric immersion for tabletop gaming*
