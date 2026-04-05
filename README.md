# Crit VTT

**Turn your TV into a living game map.**

Crit VTT is a free, open-source virtual tabletop built for in-person D&D sessions. Connect a TV as a second display, load a map, and control what your players see with fog of war, dynamic lighting, and atmospheric weather effects. No accounts, no internet, no subscriptions — just maps on a TV, with atmosphere that makes your table feel like another world.

---

## Table of Contents

- [Quick Start](#quick-start)
- [Installation](#installation)
- [Setup & First Launch](#setup--first-launch)
- [Loading Maps](#loading-maps)
- [The DM Window](#the-dm-window)
- [The Player Window](#the-player-window)
- [Fog of War](#fog-of-war)
- [Grid Overlay](#grid-overlay)
- [Beacons](#beacons)
- [Map Browser](#map-browser)
- [Atmosphere System](#atmosphere-system)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [System Requirements](#system-requirements)
- [Troubleshooting](#troubleshooting)
- [What Crit VTT Does NOT Do](#what-crit-vtt-does-not-do)
- [Contributing](#contributing)
- [License](#license)

---

## Quick Start

1. **Connect your TV** as a second display (extended desktop, not mirrored)
2. **Launch Crit VTT**
3. **Drag a map image** onto the window (PNG, JPG, or .dd2vtt from DungeonDraft)
4. **Press F** to enable Fog of War — the map goes black on the TV
5. **Click and drag** on the map to reveal areas as players explore
6. **Press P** to open the Player Window, then drag it to your TV
7. **Double-click** anywhere to place a beacon that both you and your players can see

That's the whole workflow. Everything else is optional.

---

## Installation

### macOS

#### Download
Download the latest `.dmg` from the [Releases page](https://github.com/themightyzq/LocalVTT/releases). Open the DMG and drag Crit VTT to your Applications folder. On first launch, macOS may block the app — right-click it, select "Open", then click "Open" in the dialog. You only need to do this once.

#### Build from Source
```bash
# Install dependencies
brew install qt cmake

# Clone and build
git clone https://github.com/themightyzq/LocalVTT.git
cd LocalVTT
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build -j$(sysctl -n hw.ncpu)

# Run
open build/CritVTT.app
```

### Linux

#### Build from Source

Install Qt 6 and CMake for your distribution:

```bash
# Ubuntu / Debian
sudo apt install qt6-base-dev libqt6svg6-dev qt6-multimedia-dev libqt6opengl6-dev libgl1-mesa-dev cmake build-essential

# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtsvg-devel qt6-qtmultimedia-devel cmake gcc-c++

# Arch
sudo pacman -S qt6-base qt6-svg qt6-multimedia cmake
```

Then build:
```bash
git clone https://github.com/themightyzq/LocalVTT.git
cd LocalVTT
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run
./build/CritVTT
```

### Windows

Download pre-built binaries from the [Releases page](https://github.com/themightyzq/LocalVTT/releases), or build from source with Qt 6 for Windows and Visual Studio 2022.

---

## Setup & First Launch

### Display Setup

Connect your TV or projector as a **second display** using extended desktop mode (not mirrored). Any display arrangement works — the Player Window can be dragged to any screen. Crit VTT also works with a single monitor; the Player Window simply opens as a regular window.

### First Launch

When you launch Crit VTT, the DM window appears with an empty canvas. All toolbar buttons are visible but fog tools are grayed out until you load a map. The Player Window is closed by default — toggle it with **P** when you're ready.

---

## Loading Maps

### Supported Formats

| Format | Extensions | Notes |
|--------|-----------|-------|
| PNG | .png | Recommended for quality |
| JPEG | .jpg, .jpeg | Good for large maps |
| WebP | .webp | Modern format, small files |
| BMP | .bmp | Supported but large |
| GIF | .gif | Displays first frame |
| DungeonDraft | .dd2vtt | Full support — grid auto-configured from metadata |
| Universal VTT | .uvtt | Metadata support |
| FoundryVTT | .df2vtt | Basic support |

### How to Load

- **Drag and drop** an image or VTT file onto the window
- **File > Open** (Ctrl+O / Cmd+O)
- **Map Browser** (B key) to browse your collection with thumbnails

### Multiple Maps

Each map opens in its own tab. Click tabs to switch — each tab preserves its own map, fog state, grid settings, and zoom level. Hover over a tab to see a thumbnail preview. Close tabs you don't need to free memory.

---

## The DM Window

### Toolbar

From left to right:

| Button | What It Does |
|--------|-------------|
| **Load Map** | Open a file dialog to load a map |
| **Fog Mode** | Master switch — turns fog of war on or off |
| **REVEAL / HIDE** | Toggle between revealing and hiding fog (green = reveal, red = hide) |
| **Brush** | Select the circular brush for painting fog |
| **Rectangle** | Select the rectangle tool for revealing rooms |
| **Reset Fog** | Cover the entire map with fog again (requires confirmation) |
| **Zoom Out / In / Fit** | Control the DM's view zoom |
| **Center** | Re-center the view on the map |
| **Rotate** | Rotate the map 90 degrees |
| **Zoom spinner** | Set exact zoom percentage (10–500%) |
| **Grid** | Toggle the grid overlay |
| **Player View** | Open or close the Player Window |

Fog tools are grayed out when Fog Mode is off. All controls are always visible — nothing is hidden.

### Navigation

**Panning:**
- Scroll to pan vertically, Shift+scroll to pan horizontally
- Hold **Spacebar** and drag anywhere (recommended for trackpads)
- Middle-click and drag, or Shift+right-click and drag

**Zooming:**
- **Ctrl/Cmd + scroll** to zoom in and out
- **+** / **-** keys, or **/** to fit the map in the window
- **Ctrl/Cmd + 1/2/3** to jump to 100%, 200%, 300%

Plain scroll pans instead of zooming to prevent accidental zoom on trackpads.

---

## The Player Window

This is the window you put on your TV for players to see. Toggle it with **P** or the Player View toolbar button, then drag it to your TV and maximize or fullscreen it.

**What players see:**
- The map with solid black fog over unrevealed areas
- Revealed areas appear as the DM paints them
- Beacons placed by the DM
- Atmosphere effects (lighting, weather, fog/mist)
- No buttons, no UI — just the map

**What players don't see:**
- The DM's zoom and pan position (Player Window auto-fits the map)
- The grid overlay
- Anything behind the fog

**View sync:** Press **V** to push your current view to the Player Window. Press **Shift+V** to reset it back to auto-fit.

---

## Fog of War

### How It Works

When Fog Mode is on, the entire map is covered in black on the Player Window. The DM reveals areas by painting through the fog. On the DM's screen, fogged areas appear dimly so you can always see the full map.

### Revealing and Hiding

1. Press **F** to turn on Fog Mode
2. Select **Brush** (circular) or **Rectangle** from the toolbar
3. Click and drag to reveal areas

To paint fog back over an area, press **H** to switch to Hide mode. The toolbar indicator changes from green "REVEAL" to red "HIDE". Press **H** again to switch back.

### Brush Size

- Use the Brush Size spinner (10–400 pixels)
- Press **[** to decrease and **]** to increase quickly

### Undo and Redo

- **Ctrl+Z** (Cmd+Z on Mac) to undo the last fog stroke
- **Ctrl+Y** (Cmd+Y on Mac) to redo
- Each click-and-drag is one undo step

### Reset Fog

Click **Reset Fog** to cover the entire map again. A confirmation dialog appears — this cannot be undone.

### Tips

- Reveal rooms just before players enter for maximum impact
- Use Rectangle for clean room reveals, Brush for caves and natural shapes
- Fog state is saved per tab — switching tabs preserves your reveals

---

## Grid Overlay

Toggle the grid with **G** or the Grid toolbar button. Adjust the size with the Grid Size spinner (20–500 pixels, default 150). The grid appears on the DM window only. VTT files from DungeonDraft automatically set the grid to match the map's built-in grid.

---

## Beacons

Double-click anywhere on the map to place a beacon. Beacons are visible on both the DM and Player windows — use them to direct player attention ("You see a light here" or "The entrance is here").

Beacons are animated markers that pulse briefly and then fade away. They come in 9 colors: Red (default), Blue, Yellow, Green, Orange, Purple, White, Grey, and Black. To change the beacon color before placing one, open the Tools panel and select a color from the Beacon Color section.

---

## Map Browser

Press **B** to open the Map Browser sidebar. It shows thumbnail previews of your maps organized into sections:

- **Recent Files** — maps you've recently opened, with thumbnails
- **Favorites** — maps you've starred for quick access (press F to toggle)
- **Folder Browser** — navigate your file system with thumbnail previews

Double-click any map to load it. Use the search bar to filter by name, and sort by name, date, or size. Switch between grid and list view modes. Right-click files for options like Reveal in Finder, Rename, and Delete.

---

## Atmosphere System

Press **A** to open the Atmosphere panel. It controls lighting, weather, and environmental effects that appear on both windows.

### Presets

One-click scene changes with smooth animated transitions:

| Preset | Description |
|--------|-------------|
| Peaceful Day | Bright natural light — calm outdoor scenes |
| Golden Dawn | Warm orange sunrise |
| Warm Dusk | Orange-red sunset |
| Moonlit Night | Cool blue darkness |
| Stormy Night | Dark with heavy rain and lightning |
| Light Rain | Gentle overcast drizzle |
| Winter Snow | Cold blue with snowfall |
| Dungeon Depths | Very dark underground |
| Mystical Fog | Purple ethereal mist |
| Volcanic | Red-orange heat haze |

Save your own custom presets via **Presets > Save Current as Preset**.

### Time of Day

Manual lighting control: Dawn, Day, Dusk, Night. Transitions are smooth and animated.

### Weather

- **Rain** — falling particles with adjustable intensity
- **Snow** — drifting snowflakes
- **Storm** — heavy rain with multi-flash lightning strikes

### Fog and Mist

Animated multi-layer fog that drifts across the map. This is a visual atmosphere effect, separate from the fog of war system.

### Point Lights

Place dynamic lights on your map:

1. Go to **View > Atmosphere > Lights > Place Light Mode**
2. Click the map to place a light
3. Click a light to select it (cyan ring), drag to move
4. Double-click to edit properties (color, radius, intensity, flicker)
5. Delete/Backspace to remove

Light presets: Torch (flickering orange), Lantern (steady warm), Campfire (large flicker), Magical (blue pulse), Moonbeam (pale silver).

### Atmosphere Tips

- Apply a preset **before** revealing areas — the first impression hits hardest
- Transition between presets mid-session: Peaceful Day → Stormy Night as danger approaches
- Layer point lights on dark presets: Dungeon Depths + torches = perfect dungeon

---

## Keyboard Shortcuts

### General

| Key | Action |
|-----|--------|
| Ctrl/Cmd+O | Open map file |
| P | Toggle Player Window |
| B | Toggle Map Browser |
| A | Toggle Atmosphere Panel |
| L | Toggle lighting |
| F1 | Keyboard shortcuts overlay |
| F12 | Debug console |
| Escape | Cancel / deselect |

### Fog of War

| Key | Action |
|-----|--------|
| F | Toggle Fog Mode |
| H | Toggle Reveal / Hide mode |
| [ | Decrease brush size |
| ] | Increase brush size |
| Ctrl/Cmd+Z | Undo fog change |
| Ctrl/Cmd+Y | Redo fog change |

### View & Navigation

| Key | Action |
|-----|--------|
| + or = | Zoom in |
| - | Zoom out |
| / | Fit map to window |
| Ctrl/Cmd+1/2/3 | Zoom to 100% / 200% / 300% |
| Spacebar (hold) | Pan mode — drag to move |
| G | Toggle grid |
| V | Push DM view to Player Window |
| Shift+V | Reset Player Window to auto-fit |

### Mouse & Trackpad

| Action | Result |
|--------|--------|
| Click + drag | Paint fog reveal/hide |
| Double-click | Place beacon |
| Scroll | Pan vertically |
| Shift + scroll | Pan horizontally |
| Ctrl/Cmd + scroll | Zoom |
| Middle-click drag | Pan |
| Spacebar + drag | Pan |

---

## System Requirements

| | Minimum | Recommended |
|--|---------|-------------|
| **OS** | macOS 11+, Linux (X11), Windows 10+ | macOS 13+, Ubuntu 22.04+ |
| **RAM** | 4 GB | 8 GB |
| **Graphics** | OpenGL 3.3 | Dedicated GPU |
| **Display** | 1920x1080 | 4K TV for player view |
| **Setup** | Single monitor | Dual display (computer + TV) |

---

## Troubleshooting

### The app won't launch on macOS

macOS blocks apps from unidentified developers. Right-click the app, select "Open", then click "Open" in the dialog. You only need to do this once.

### JPEG or WebP maps show a blank screen

The image format plugins may be missing. If you built from source, run `macdeployqt` on the .app bundle. If you downloaded a release, please [file a bug](https://github.com/themightyzq/LocalVTT/issues).

### Player Window is on the wrong screen

Drag the Player Window to the correct display and maximize it. The app remembers the position.

### Fog isn't showing on the Player Window

Make sure Fog Mode is on (press F). It's a master switch — when off, the full map is visible on both windows.

### Performance is slow with large maps

Maps larger than 16384 pixels on any side are automatically scaled down. For best performance, use maps under 4096x4096 pixels and disable atmosphere effects you aren't using.

### Where are settings stored?

- **macOS:** ~/Library/Application Support/CritVTT/
- **Linux:** ~/.local/share/CritVTT/

### Where are log files?

- **macOS:** ~/Library/Application Support/CritVTT/logs/critvtt.log
- **Linux:** ~/.local/share/CritVTT/logs/critvtt.log

### Coming from an older version (Project VTT)?

Your settings are automatically migrated on first launch. If something went wrong, your old data is still at ~/Library/Application Support/ProjectVTT/.

---

## What Crit VTT Does NOT Do

Crit VTT does one thing well: atmospheric maps for in-person games. Here's what it intentionally leaves out:

| Not Included | Why | Try Instead |
|---|---|---|
| Online play | Built for real tables | Roll20, Foundry VTT, Owlbear Rodeo |
| Dice rolling | Physical dice are more fun | Real dice, D&D Beyond |
| Character sheets | Not a game manager | D&D Beyond, paper |
| Combat tracking | Theater of the mind | Initiative trackers, pen & paper |
| Measurement | Handle it at the table | Count grid squares, use a ruler |
| Music / audio | Use a dedicated app | Spotify, Syrinscape, Tabletop Audio |
| Drawing / notes | Keep the map clean | Pen & paper, OneNote |

---

## Contributing

Bug reports and feature requests are welcome on the [GitHub Issues page](https://github.com/themightyzq/LocalVTT/issues).

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

*Your table. Your dice. Your TV becomes another world.*
