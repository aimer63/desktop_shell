# Fork motivation

## Motivation

Fork `tray_manager` and merge with essential `window_manager` features to create
a unified, plugin.

## Issues with tray_manager

### Windows: Menu Dismissal

**Problem:** Clicking outside the tray menu doesn't close it.
**Root Cause:** Missing `SetForegroundWindow()` before `TrackPopupMenu()`.

### Windows: Light/Dark Mode

**Problem:** Menus don't follow system theme (always light).
**Root Cause:** Win32 `TrackPopupMenu()` doesn't automatically respect dark mode.
**Solution:** Use undocumented `SetPreferredAppMode()` and `FlushMenuThemes()` from uxtheme.dll.

### Linux: Deprecated API

**Problem:** Uses deprecated `libappindicator`.
**Status:** Need to decide migration path:

- Option 1: `libayatana-appindicator-glib` (easier migration, current compatibility)
- Option 2: StatusNotifierItem (DBus protocol, future-proof but more complex)

## Why Merge with window_manager

**Convenience:** Single plugin provides both tray and window management:

- Tray icon with context menu
- Window show/hide/focus
- Close button interception
- Minimize to tray

**Removed from window_manager:**

- Full screen mode
- Window resizing
- Title bar customization
- Always on top
- Window transparency
- Multi-window support

> **_Note:_ We consider reintroducing features in future**

## Scope

**Included:**

- System tray icon and menu
- Window show/hide/focus/close interception
- Unified Result-based API
- Windows, Linux, macOS support

**Excluded:**

- Complex window features (see above)
- Animations
- Multi-window support
