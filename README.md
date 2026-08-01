# desktop_shell

A unified Flutter desktop plugin combining system tray and window management.

## Overview

This plugin merges [tray_manager](https://github.com/leanflutter/tray_manager)
and [window_manager](https://github.com/leanflutter/window_manager) into a
single, cohesive API for desktop Flutter applications.

## Attribution

This project is built upon two excellent MIT-licensed projects:

- **[tray_manager](https://github.com/leanflutter/tray_manager)** by leanflutter
  - System tray functionality
- **[window_manager](https://github.com/leanflutter/window_manager)**
  by leanflutter - Window management

Both original projects are MIT licensed. See [LICENSE](LICENSE) for full
attribution details.

## Features

- System tray icon with context menu
- Window show/hide/focus operations
- Minimize-to-tray behavior
- Windows, Linux, and macOS support

## Structure

```
desktop_shell/
├── packages/
│   ├── tray_manager/     # Original tray_manager package
│   └── window_manager/   # Original window_manager package
├── doc/
│   └── tray-manager-fork.md  # Implementation plan and documentation
├── LICENSE
└── README.md
```

## Linux Implementation Notes

### System Tray Library

This plugin uses `libayatana-appindicator` (GTK-based) for system tray
functionality on Linux.

#### Deprecation Warning

During compilation, you may see a deprecation warning:

```
warning: 'app_indicator_new' is deprecated [-Wdeprecated-declarations]
```

**Why this happens:** The constructor `app_indicator_new()` is marked as
deprecated in the library headers, but no replacement constructor is provided by
the library maintainers.

**Impact:** This is a warning only (not an error). The build will succeed and
the plugin will work correctly. The warning serves as documentation that the API
is deprecated, but since no alternative exists, it remains functional.

#### Attempted Migration to libayatana-appindicator-glib

We attempted to migrate to the newer `libayatana-appindicator-glib` library
which is advertised as a modern, GLib-only replacement that eliminates GTK
dependencies.

**Why it was suspended:** The new library uses a different DBus protocol
(`org.gtk.Menus` + `org.gtk.Actions`) instead of the legacy
`com.canonical.dbusmenu` protocol. Current desktop environments (GNOME, XFCE)
only support the old protocol, so menus exported via the new protocol are never
displayed.

**Evidence:**

- GNOME Shell requests menu via: `com.canonical.dbusmenu`
- New library exports via: `org.gtk.Menus`
- Upstream issue: [Issue 102](https://github.com/AyatanaIndicators/libayatana-appindicator-glib/issues/102)

**Status:** Migration suspended until major desktop environments implement
support for the new protocol. We remain on the GTK-based
`libayatana-appindicator` which works correctly across all Linux distributions.

For full technical details, see
[doc/libayatana-appindicator-glib.md](doc/libayatana-appindicator-glib.md).

## License

MIT License. See [LICENSE](LICENSE) for details.
