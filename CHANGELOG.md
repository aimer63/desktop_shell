# Changelog

All notable changes to the desktop_shell plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Unified Dart API**: Complete redesign with `unwrap_me` Result/Option types for
  explicit error handling. All operations return `Result<(), ErrorType>` instead of
  throwing exceptions. Sealed error hierarchy with detailed error messages.
- **Single platform channel**: Unified `'desktop_shell'` channel replacing separate
  `tray_manager` and `window_manager` channels. All tray and window operations
  go through one channel with consistent error handling.
- **Merged native implementations**:
  - **Windows**: Unified `desktop_shell_plugin.cpp` with integrated tray icon and
    window management. Supports tray icon click events, context menus, window
    show/hide/focus, and close interception.
  - **Linux**: StatusNotifierItem-based tray implementation (replacing deprecated
    libappindicator). GTK-based window management with close interception.
  - **macOS**: Swift-based implementation with NSStatusBar tray and NSWindow
    management.
- **Simplified API surface**: Only essential features for minimize-to-tray workflow:
  - `initialize()` - Single call to set up tray and window with callbacks
  - `setTrayIcon()`, `setTrayMenu()`, `popUpContextMenu()`
  - `showWindow()`, `hideWindow()`, `focusWindow()`
  - `setPreventClose()`, `destroy()`
- **Event callbacks**: `onWindowClose` and `onTrayIconClick` callbacks passed to
  `initialize()` for handling user interactions.
- **Menu system**: Inlined `Menu` and `MenuItem` classes (removed `menu_base`
  dependency) with support for separators and click handlers.
- **Analysis configuration**: `analysis_options.yaml` with `flutter_lints` and
  exclusion of archived packages.
- **Suppressed deprecation warning in Linux tray**:
  - Added `#pragma GCC diagnostic ignored` around `app_indicator_new()` call
  - Library marks constructor as deprecated but provides no replacement
  - Warning suppressed only for this specific call, not globally
  - Build now completes without warnings while maintaining functionality
- **Documentation updates**:
  - Updated README.md with Linux implementation notes section
  - Documented deprecation warning and libayatana-appindicator-glib migration attempt
  - Added link to full technical details in doc/libayatana-appindicator-glib.md

### Changed

- **Package structure**: Merged from monorepo (tray_manager + window_manager packages)
  to single unified plugin. Old packages moved to `packages/_archive/`.
- **Error handling**: Migrated from exception-based to Result-based (unwrap_me).
  All errors are typed values in `Result<(), ErrorType>` format.
- **pubspec.yaml**: Replaced workspace configuration with single package config.
  Added `unwrap_me` git dependency, removed `menu_base` and `screen_retriever`.
- **Type conventions**: Changed `Result<void, E>` to `Result<(), E>` using empty
  record type as Unit type (consistent with Chans UI patterns).

### Removed

- **Monorepo structure**: Removed `melos.yaml` and workspace configuration.
- **Unused dependencies**: `menu_base`, `screen_retriever`, `ffi` (not needed
  for current feature set).
- **Unused features** (from original packages):
  - Window resizing, minimizing, maximizing, fullscreen
  - Title bar customization
  - Always on top/bottom
  - Window transparency and opacity
  - Window positioning and bounds
  - Multi-window support
  - Tray tooltip, title, icon position (macOS-specific)
  - All widget classes (window_caption, drag areas, etc.)

## [0.1.0] - 2026-07-25

### Added

- Initial release of unified desktop_shell plugin.
- Forked and merged code from tray_manager and window_manager (both MIT licensed
  by leanflutter/LiJianying).
- Attribution in LICENSE and README to original authors.
