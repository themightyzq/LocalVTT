# Project VTT - Living Maps for Tabletop Gaming

![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-blue)

## What is Project VTT?

Transform your TV into a living game map for **in-person** tabletop RPGs. Project VTT displays maps with fog of war control, letting you reveal areas as players explore.

**Design Philosophy:** Simple and visual. Click buttons and watch your world come alive.

---

## Quick Start (30 seconds)

1. Connect TV as second display
2. Launch Project VTT
3. Drag map image onto window
4. Click **[Fog Mode]** - Map goes black on TV
5. Click **[Reveal Brush]** - Paint to reveal areas
6. Double-click anywhere - Beacon appears for players

**That's it.** No tutorial needed.

---

## Core Features

### Fog of War
- **Fog Mode toggle** - Master switch (on/off)
- **Reveal Brush** - Paint revealed areas (circle, default 200px)
- **Hide Brush** - Paint fog back over areas (H key to toggle)
- **Reveal/Hide Rectangle** - Drag to reveal or hide rooms
- **Reset Fog** - Start over (confirmation required)
- **Auto-sync** - DM and player windows always in sync

### Map Display
- **Drag & drop** - PNG, JPG, WebP images
- **VTT files** - DD2VTT, UVTT, DF2VTT support
- **Grid overlay** - Adjustable size (default 150px)
- **Multi-tab** - Up to 10 maps, instant switching

### DM Tools
- **Beacon** - Double-click to direct player attention
- **Dual display** - DM control + TV player view
- **Recent files** - Quick access to last 10 maps
- **Zoom controls** - Buttons + spinner (10-500%)

### Map Browser (B key)
- **Recent files** - Thumbnail previews of recently opened maps
- **Favorites** - Star maps for quick access (F key to toggle)
- **Folder browsing** - Navigate your map collection with thumbnails
- **Search** - Filter files in current folder
- **Drag & drop** - Drag maps directly onto the main window
- **Sorting** - By name, date modified, or file size
- **View modes** - Grid or list view
- **Folder bookmarks** - Save frequently used folders
- **Context menu** - Right-click for Reveal in Finder, Rename, Delete, File Info
- **Batch operations** - Multi-select with Ctrl/Shift for bulk actions

### Atmosphere System
Create dramatic "wow moments" with lighting and weather effects.

- **10 Presets** - Peaceful Day, Golden Dawn, Warm Dusk, Moonlit Night, Stormy Night, Light Rain, Winter Snow, Dungeon Depths, Mystical Fog, Volcanic
- **Lighting** - Smooth animated transitions between time-of-day settings
- **Weather** - Rain, snow, and storm particle effects
- **Fog/Mist** - Animated multi-layer fog overlay
- **Point Lights** - Click-to-place torches, lanterns, campfires, magical lights, moonbeams

---

## Toolbar

```
[Load Map] | [Fog Mode] [Brush] [Rect] [Reset Fog] |
[Zoom Out] [Zoom In] [Fit] | [Grid] | [Player View] |
Brush: [spinner] Grid: [spinner] Zoom: [spinner]
```

| Control | Range | Default |
|---------|-------|---------|
| Brush Size | 10-400px | 200px |
| Grid Size | 20-500px | 150px |
| Zoom Level | 10-500% | 100% |

All controls always visible. Disabled (grayed) when inactive.

---

## Controls

### Mouse & Trackpad Navigation
| Action | Behavior |
|--------|----------|
| Single click | Reveal fog / Select button |
| Drag | Paint brush / Draw rectangle |
| Double-click | Place beacon (works everywhere) |
| Scroll (two-finger) | Pan map vertically |
| Shift + Scroll | Pan map horizontally |
| Ctrl/Cmd + Scroll | Zoom in/out |
| Middle-click drag | Pan map |
| Spacebar + drag | Pan map (trackpad-friendly) |
| Shift + Right-drag | Pan map (alternative) |

### Keyboard
| Key | Action |
|-----|--------|
| B | Toggle Map Browser |
| F | Toggle Fog Mode |
| G | Toggle Grid |
| H | Toggle Fog Hide Mode (reveal/hide) |
| +/- | Zoom in/out |
| / | Fit to window |
| P | Toggle Player View |
| A | Toggle Atmosphere Panel |
| V | Sync View to Players |
| Shift+V | Reset Player Auto-Fit |
| Shift+R | Push Rotation to Players |
| Ctrl+R | Rotate Player View Only |
| Spacebar | Hold for pan mode |
| Escape | Cancel/deselect |
| Ctrl+Z/Y | Undo/Redo fog |
| Ctrl+O | Load map |
| [ / ] | Decrease/Increase brush size |
| L | Toggle lighting system |
| Ctrl+1/2/3 | Zoom to 100%/200%/300% |
| F12 | Debug console |

**Note:** Plain scroll pans (not zooms) to prevent accidental zoom on trackpads

---

## Atmosphere System

Access via **View → Atmosphere** menu.

### Presets
Quick one-click scene changes with smooth transitions:

| Preset | Description |
|--------|-------------|
| Peaceful Day | Bright natural light, no effects |
| Golden Dawn | Warm orange sunrise tones |
| Warm Dusk | Orange-red sunset atmosphere |
| Moonlit Night | Cool blue nighttime lighting |
| Stormy Night | Dark with heavy rain |
| Light Rain | Gentle overcast with rain |
| Winter Snow | Cold blue tones with falling snow |
| Dungeon Depths | Dark underground with thick shadows |
| Mystical Fog | Ethereal purple mist |
| Volcanic | Red-orange heat effects |

### Time of Day
Manual lighting control: Dawn, Day, Dusk, Night

### Point Lights
Place dynamic light sources on your map:

1. Go to **View → Atmosphere → Lights → Place Light Mode**
2. **Click** on the map to place a light
3. **Click** an existing light to select it (cyan ring appears)
4. **Drag** selected lights to reposition
5. **Delete/Backspace** to remove selected light
6. **Escape** to deselect

**Light Presets:**
| Light | Color | Effect |
|-------|-------|--------|
| Torch | Warm orange | Flickering |
| Lantern | Steady warm | No flicker |
| Campfire | Large orange | Strong flicker |
| Magical | Blue ethereal | Gentle pulse |
| Moonbeam | Pale silver | Soft glow |

---

## Supported Formats

### Images
- PNG, JPG, WebP, BMP, GIF

### VTT Files
- **DungeonDraft** (.dd2vtt) - Full support
- **Universal VTT** (.uvtt) - Metadata support
- **FoundryVTT** (.df2vtt) - Basic support

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

## Building from Source

```bash
# Build and run
./build-and-run.command
```

**Requirements:**
- Qt 6.6+ (with Multimedia module)
- CMake 3.16+
- C++17 compiler

---

## What This App Does NOT Do

Project VTT is laser-focused on **maps and fog**:

- No online/multiplayer (in-person only)
- No dice rolling (use physical dice)
- No character sheets (use D&D Beyond/paper)
- No combat tracking (theater of mind)
- No measurement tools (handle at table)
- No audio system (use external app)
- No drawing/notes (keep it simple)

**Why?** Every feature removed is one less thing that can break.

---

## Documentation

- **CLAUDE.md** - Development guidelines
- **TODO.md** - Task tracking
- **ROADMAP.md** - Feature ideas

---

## Contributing

1. Read `CLAUDE.md` (development rules)
2. "If it takes >5 seconds to explain, it's too complex"
3. Test manually
4. Minimal diffs only

---

## License

MIT License - See [LICENSE](LICENSE) for details

---

**Your table. Your dice. Your TV becomes another world.**

*Project VTT - Atmospheric immersion for tabletop gaming*
