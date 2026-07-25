# desktop_shell

A unified Flutter desktop plugin combining system tray and window management.

## Overview

This plugin merges [tray_manager](https://github.com/leanflutter/tray_manager) and [window_manager](https://github.com/leanflutter/window_manager) into a single, cohesive API for desktop Flutter applications.

## Attribution

This project is built upon two excellent MIT-licensed projects:

- **[tray_manager](https://github.com/leanflutter/tray_manager)** by leanflutter - System tray functionality
- **[window_manager](https://github.com/leanflutter/window_manager)** by leanflutter - Window management

Both original projects are MIT licensed. See [LICENSE](LICENSE) for full attribution details.

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

## License

MIT License. See [LICENSE](LICENSE) for details.
