# Changelog

## v1.0.1 — May 2026

End-user audit cleanup. No new features.

### Fixed
- Audio panel removed — `QMediaPlayer` failed to initialize on most installs and the panel showed dead controls. Audio is now fully out of scope, matching the README.
- Documented `F11` (Player Window fullscreen) and `Ctrl+Shift+F` (Player auto-fit) shortcuts in CLAUDE.md §7.1, README, and the F1 overlay.
- Resolved `Ctrl+1/2/3` shortcut collision: zoom presets keep these combos; recent files moved to `Ctrl+Shift+1..9`.
- Removed undocumented `R` shortcut for Rectangle Fog tool (CLAUDE.md §7.1 forbids undocumented shortcuts).
- F1 keyboard overlay now lists `Shift+R`, `Ctrl+R`, `Ctrl+Shift+1..9`, `F11`, and `Ctrl+Shift+F`.
- About dialog and in-app Quick Start text updated to match README (press P for Player Window).
- Help → Quick Start menu entry now wired up (was implemented but unreachable).
- `ActionRegistry` no longer claims `Ctrl+Shift+F` and `Ctrl+Shift+R` for fog actions — they collided with new §7.1 entries / were undocumented. Reset Fog and Clear Fog remain available via toolbar and menus.
- `setAudioSystems()` hardened with an early-return guard so the audio panel can never crash on a nullptr widget if real audio systems are ever reattached.
- F1 keyboard overlay card grew to fit the new Display column without crowding the dismiss hint.
- Fog brush/rect tooltips clarified ("H toggles Hide/Reveal" instead of the ambiguous "H to toggle mode").

### Docs / Distribution
- README GitHub URLs corrected (LocalVTT → CritVTT).
- Release DMG renamed `ProjectVTT-1.0.0-macOS.dmg` → `CritVTT-1.0.0-macOS.dmg`.
- Sweep of "Project VTT" → "Crit VTT" across BUILD_README, MANUAL_TESTING, CLEAN_MACHINE_TEST, ARCHITECTURE, USER_GUIDE.
- Stale `build/ProjectVTT.app` and `build-release/` directories removed.
- `RELEASE_AUDIT_REPORT.md` moved out of repo root into `docs/development/`.

---

## v1.0.0 — April 2026

First public release as **Crit VTT** (renamed from Project VTT).

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
