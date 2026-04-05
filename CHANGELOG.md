# Changelog

## v1.0.0 — April 2026

First public release.

### Core Features
- **Dual-window system** — DM control window + Player TV display
- **Fog of War** — Reveal brush, hide brush (H key), rectangle reveal/hide, undo/redo
- **Grid overlay** — Adjustable size (20-500px), toggle with G key
- **Beacons** — Double-click to place, 9 color variants
- **Multi-tab maps** — Up to 10 maps with instant switching
- **Zoom & pan** — Ctrl+scroll zoom, spacebar+drag pan, trackpad-friendly

### Atmosphere System
- 10 built-in presets with smooth animated transitions
- Dynamic lighting (Dawn/Day/Dusk/Night) with tint and intensity
- Weather effects (Rain, Snow, Storm) with particle systems
- Fog/mist overlay with animated texture layers
- Lightning flash effects
- Point light system with flicker animation
- Custom preset save/load

### Audio System
- Ambient sound engine with crossfade between tracks
- System music remote (Spotify/Apple Music control on macOS)
- Audio controls in Atmosphere panel

### Map Support
- PNG, JPG, WebP, BMP, GIF images
- DD2VTT, UVTT, DF2VTT virtual tabletop files
- Map Browser panel with thumbnails, favorites, folder navigation
- Drag & drop loading

### Quality
- Dark theme with consistent styling (DarkTheme component system)
- Keyboard shortcuts overlay (F1)
- Player Window right-click context menu
- File-based crash logging
- Zero compiler warnings (-Wall -Wextra -Wpedantic)
- 16384px max image dimension protection
- Reentrancy-safe image loading

### Performance
- Unified 30fps animation driver (SceneAnimationDriver)
- Grid pixmap cache with batched line rendering
- Fog mist pre-render to offscreen pixmap
- Snow particle batching (drawRects)
- Fog paint throttling during brush strokes
