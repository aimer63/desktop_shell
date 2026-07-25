# tray_manager Fork Plan

## Overview

Create a unified Flutter desktop plugin by forking `tray_manager` and integrating essential window management features. This solves all current issues while providing a clean, maintainable API specifically for Chans.

**Repository:** `github.com/aimer63/desktop_shell`

## Why Fork?

### Current State

- `tray_manager`: Abandoned, Windows menu issues, deprecated Linux API
- `window_manager`: Over-engineered, only need 20% of features
- Using both: Coordination issues, dependency bloat

### After Fork

- Single plugin: `desktop_shell`
- Unified API: Tray + Window in one interface
- Fixed issues: Windows theme, menu dismissal, Linux deprecation
- Minimal scope: Only what Chans needs

## Requirements

### Essential Features (Must Have)

| Feature | Current Package | Priority |
| --------- | ----------------- | ---------- |
| System tray icon | tray_manager | Required |
| Tray context menu | tray_manager | Required |
| Menu item callbacks | tray_manager | Required |
| Window show/hide | window_manager | Required |
| Window focus | window_manager | Required |
| Intercept close button | window_manager | Required |
| Minimize to tray | Both | Required |

### Optional Features (Nice to Have)

| Feature | Current Package | Priority |
| --------- | ----------------- | ---------- |
| Window position restore | window_manager | Low |
| Tray tooltip | tray_manager | Low |
| Menu icons | tray_manager | Low |

### Not Needed (Remove)

- Full screen mode
- Window resizing
- Title bar customization
- Always on top
- Window transparency
- Multi-window support

## Architecture

### Plugin Structure

```
desktop_shell/
├── lib/
│   ├── desktop_shell.dart          # Main export
│   └── src/
│       ├── desktop_shell.dart      # Main API class
│       ├── tray/
│       │   ├── tray_icon.dart      # Tray icon interface
│       │   └── tray_menu.dart      # Menu interface
│       └── window/
│           └── window_manager.dart # Window interface
├── windows/
│   ├── CMakeLists.txt
│   └── desktop_shell_plugin.cpp    # Windows implementation
├── linux/
│   ├── CMakeLists.txt
│   └── desktop_shell_plugin.cc     # Linux implementation
├── macos/
│   └── Classes/
│       └── DesktoShellPlugin.swift # macOS implementation
├── example/                         # Test app
└── pubspec.yaml
```

### Dart API Design

```dart
import 'package:desktop_shell/desktop_shell.dart';
import 'package:unwrap_me/unwrap_me.dart';

// Initialize once at app startup with tray menu handlers
final initResult = await initialize(
  trayIcon: Platform.isWindows ? 'assets/icon.ico' : 'assets/icon.png',
  trayItems: [
    // Click "Open" in tray menu -> show and focus window
    TrayItem(
      label: 'Open',
      onClick: (desk) async {
        await desk.showWindow();
        await desk.focusWindow();
      },
    ),
    TrayItem.separator(),
    // Click "Quit" in tray menu -> cleanup and exit
    TrayItem(
      label: 'Quit',
      onClick: (desk) async {
        final result = await desk.quit();
        if (result is Ok) exit(0);
      },
    ),
  ],
  // User clicks X button -> hide to tray (don't exit)
  onWindowClose: (desk) => desk.hideWindow(),
);

// Handle initialization result
switch (initResult) {
  case Ok(:final value):
    final desk = value;
    runApp(MyApp(desk: desk));
  case Err(:final error):
    stderr.writeln('Failed to initialize: ${error.message}');
    exit(1);
}
```

### API Classes

```dart
import 'package:unwrap_me/unwrap_me.dart' show Result, Ok, Err;

/// Initialize desktop shell with tray and window management.
/// Returns Result containing DesktoShell instance or error.
Future<Result<DesktoShell, DesktoShellError>> initialize({
  required String trayIcon,
  required List<TrayItem> trayItems,
  required void Function(DesktoShell desk) onWindowClose,
});

/// Main desktop shell controller.
/// All operations return Result for explicit error handling.
abstract class DesktoShell {
  /// Show window from tray.
  Future<Result<void, WindowOperationError>> showWindow();
  
  /// Hide window to tray.
  Future<Result<void, WindowOperationError>> hideWindow();
  
  /// Focus window.
  Future<Result<void, WindowOperationError>> focusWindow();
  
  /// Quit application and cleanup.
  Future<Result<void, DesktoShellError>> quit();
}

/// Tray menu item with click handler.
/// No nullable fields - separator has empty label but that's valid.
final class TrayItem {
  final String label;
  final void Function(DesktoShell desk) onClick;
  final bool isSeparator;
  
  const TrayItem({
    required this.label,
    required this.onClick,
    this.isSeparator = false,
  });
  
  const TrayItem.separator()
    : label = '',
      onClick = _noop,
      isSeparator = true;
      
  static void _noop(DesktoShell _) {}
}

/// Result types for desktop_shell operations.
sealed class DesktoShellResult {
  const DesktoShellResult();
}

/// Errors that can occur during desktop_shell operations.
sealed class DesktoShellError {
  const DesktoShellError();
  String get message;
}

/// Failed to initialize tray icon.
final class TrayInitError extends DesktoShellError {
  final String details;
  
  const TrayInitError(this.details);
  
  @override
  String get message => 'Tray initialization failed: $details';
}

/// Failed to initialize window management.
final class WindowInitError extends DesktoShellError {
  final String details;
  
  const WindowInitError(this.details);
  
  @override
  String get message => 'Window initialization failed: $details';
}

/// Platform is not supported.
final class UnsupportedPlatformError extends DesktoShellError {
  final String platform;
  
  const UnsupportedPlatformError(this.platform);
  
  @override
  String get message => 'Platform not supported: $platform';
}

/// Window operation failed.
sealed class WindowOperationError extends DesktoShellError {
  const WindowOperationError();
}

/// Failed to show window.
final class WindowShowError extends WindowOperationError {
  final String details;
  
  const WindowShowError(this.details);
  
  @override
  String get message => 'Failed to show window: $details';
}

/// Failed to hide window.
final class WindowHideError extends WindowOperationError {
  final String details;
  
  const WindowHideError(this.details);
  
  @override
  String get message => 'Failed to hide window: $details';
}

/// Failed to focus window.
final class WindowFocusError extends WindowOperationError {
  final String details;
  
  const WindowFocusError(this.details);
  
  @override
  String get message => 'Failed to focus window: $details';
}
```

### Usage Example

#### DesktopShellManager (Chans Wrapper)

```dart
// lib/desktop/desktop_lifecycle_manager.dart
import 'dart:io';
import 'package:desktop_shell/desktop_shell.dart';
import 'package:unwrap_me/unwrap_me.dart';

/// Manages desktop lifecycle: system tray and window behavior.
/// Thin wrapper around desktop_shell plugin for Chans-specific configuration.
class DesktopShellManager {
  static DesktopShell? _shell;
  
  /// Initialize tray icon and window behavior.
  /// All configuration is encapsulated here.
  static Future<Result<(), String>> initialize() async {
    final initResult = await initializeShell(
      trayIcon: Platform.isWindows 
          ? 'assets/images/k-32-blue-base.ico' 
          : 'assets/images/k-32-blue-base.png',
      trayItems: [
        // Click "Open" in tray menu -> show and focus window
        TrayItem(
          label: 'Open',
          onClick: (desk) async {
            await desk.showWindow();
            await desk.focusWindow();
          },
        ),
        TrayItem.separator(),
        // Click "Quit" in tray menu -> cleanup and exit
        TrayItem(
          label: 'Quit',
          onClick: (desk) => _quitApp(),
        ),
      ],
      // User clicks X button -> minimize to tray (don't exit)
      onWindowClose: (desk) => desk.hideWindow(),
    );
    
    switch (initResult) {
      case Ok(:final value):
        _shell = value;
        return Ok(());
      case Err(:final error):
        return Err('Desktop initialization failed: ${error.message}');
    }
  }
  
  /// Show window from tray.
  static Future<void> showWindow() async {
    if (_shell case final shell?) {
      final result = await shell.showWindow();
      if (result is Err) {
        // Log error but don't crash
        stderr.writeln('Failed to show window: ${result.error.message}');
      }
    }
  }
  
  /// Hide window to tray.
  static Future<void> hideWindow() async {
    if (_shell case final shell?) {
      await shell.hideWindow();
    }
  }
  
  /// Focus window.
  static Future<void> focusWindow() async {
    if (_shell case final shell?) {
      await shell.focusWindow();
    }
  }
  
  /// Cleanup and exit.
  static Future<void> quit() async {
    if (_shell case final shell?) {
      final result = await shell.quit();
      if (result is Ok) {
        exit(0);
      } else {
        stderr.writeln('Quit error: ${result.error.message}');
        exit(1);
      }
    }
  }
  
  /// Cleanup tray icon (called from main.dart dispose).
  static void destroyTray() {
    // Cleanup happens in quit(), nothing needed here
  }
}

void _quitApp() async {
  await DesktopShellManager.quit();
}
```

#### main.dart (Chans Usage)

```dart
// lib/main.dart
import 'dart:io';
import 'package:chans/desktop/desktop_lifecycle_manager.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  // Single call to initialize both tray and window
  final initResult = await DesktopShellManager.initialize();
  
  switch (initResult) {
    case Ok():
      // Initialization successful, proceed with app startup
      break;
    case Err(:final error):
      // Initialization failed
      stderr.writeln(error);
      exit(1);
  }
  
  // Continue with single-instance enforcement
  if (isDesktop) {
    final result = await _enforceSingleInstance(args);
    switch (result) {
      case Ok(value: FirstInstance()):
        // First instance - continue
        break;
      case Ok(value: SecondInstance()):
        // Second instance - show existing window and exit
        await DesktopShellManager.showWindow();
        exit(0);
      case Ok(value: UnsupportedInstance()):
        // Not supported - continue without enforcement
        break;
      case Err(:final error):
        stderr.writeln('Single instance error: $error');
        exit(1);
    }
  }
  
  runApp(const AppRoot());
}

// In _ChansState.dispose():
@override
void dispose() {
  if (isDesktop) {
    DesktopShellManager.destroyTray();
  }
  // ... other cleanup
  super.dispose();
}
```

## Platform Implementation Details

### Windows

**Current Issues:**

- Menu uses Win32 (ugly, no theme support)
- Click outside menu doesn't close it
- No dark mode support

**Solutions:**

1. **Menu Theme**

   ```cpp
   // Apply visual styles
   SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
   
   // Or use modern Windows API
   // IUIViewSettings for theme detection
   // DrawThemeBackground for themed menu
   ```

2. **Menu Dismissal**

   ```cpp
   // Fix TrackPopupMenu flags
   TrackPopupMenu(
     hMenu,
     TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_VERPOSANIMATION,
     x, y, 0, hwnd, nullptr
   );
   
   // Handle WM_KILLFOCUS properly
   case WM_KILLFOCUS:
     if (!IsChild(hwnd, (HWND)wParam)) {
       CloseMenu();
     }
     break;
   ```

3. **Modern Look**
   - Use `Shell_NotifyIcon` with `NIIF_USER` flag
   - Support HiDPI with `SetProcessDpiAwarenessContext`
   - Option: Use Windows 11 rounded corners if available

### Linux

**Current Issue:**

- Uses deprecated `app_indicator_new`
- Build warnings
- May break in future

**Solution: StatusNotifierItem**

```cpp
// Implement org.kde.StatusNotifierItem DBus interface
// More modern than appindicator
// Better Wayland support
// Used by KDE, GNOME (with extension), others

DBusInterface(
  "org.kde.StatusNotifierItem",
  "/StatusNotifierItem",
  "org.kde.StatusNotifierItem"
);

// Menu via com.canonical.dbusmenu
DBusInterface(
  "com.canonical.dbusmenu",
  "/MenuBar",
  "com.canonical.dbusmenu"
);
```

**Fallback:**

- Try StatusNotifierItem first
- Fall back to system tray if unavailable
- Document GNOME extension requirement

### macOS

**Status:** tray_manager already works well
**Changes:** Minimal, mostly API unification

## Implementation Phases

### Phase 0: Proper Forking with Attribution (Day 1)

This phase creates a new repository `desktop_shell` that properly forks and merges `tray_manager` and `window_manager` while maintaining full attribution to the original authors.

#### Prerequisites

- GitHub account
- Git installed locally
- Understanding of MIT license requirements

---

#### ✅ Step 1: Fork Both Repositories on GitHub (COMPLETED)

**Fork tray_manager:**

1. Go to <https://github.com/leanflutter/tray_manager>
2. Click the "Fork" button (top right)
3. Select your personal account or organization
4. GitHub creates: `github.com/YOUR_USERNAME/tray_manager`
5. **Important:** This maintains the "forked from" link on GitHub

**Fork window_manager:**

1. Go to <https://github.com/leanflutter/window_manager>
2. Click the "Fork" button
3. GitHub creates: `github.com/YOUR_USERNAME/window_manager`
4. This is temporary - we'll extract code from here

**Why fork instead of clone?**

- ✅ GitHub shows "forked from leanflutter/..." - public attribution
- ✅ Maintains link to original repository
- ✅ Can submit pull requests upstream if we fix bugs
- ✅ Respects MIT license (attribution requirement)
- ✅ Original author gets credit in GitHub's network graph

---

#### ✅ Step 2: Create New Repository for desktop_shell (COMPLETED)

**Create empty repository on GitHub:**

1. Go to <https://github.com/new>
2. Repository name: `desktop_shell`
3. Description: "Unified Flutter desktop plugin for tray and window management"
4. **DO NOT** initialize with README (we'll create our own)
5. **DO NOT** add .gitignore (we'll manage this)
6. **DO NOT** add license (we'll create proper MIT with attribution)
7. Click "Create repository"

---

#### ✅ Step 3: Clone and Setup Local Repository (COMPLETED)

```bash
# Create workspace directory
mkdir -p ~/devel/flutter
cd ~/devel/flutter

# Clone YOUR fork of tray_manager (this will be our base)
# You already have all the code from your fork, no need to fetch from original
git clone https://github.com/YOUR_USERNAME/tray_manager.git desktop_shell
cd desktop_shell

# Remove the origin remote (points to your tray_manager fork)
git remote remove origin

# Add your new desktop_shell repository as origin (where you'll push)
git remote add origin https://github.com/YOUR_USERNAME/desktop_shell.git

# Add window_manager remote (to copy code from)
git remote add window https://github.com/leanflutter/window_manager.git

# Fetch window_manager code (we'll extract from this)
git fetch window

# Verify remotes
git remote -v
# Should show:
# origin  https://github.com/YOUR_USERNAME/desktop_shell.git (fetch/push)
# window  https://github.com/leanflutter/window_manager.git (fetch)
```

---

#### ✅ Step 4: Clean and Prepare Repository (COMPLETED)

**Main Preparation (keep window_manager code as base):**

```bash
# We're starting from window_manager fork (master branch)
# Clean up GitHub-specific files we'll rebuild
rm -rf .github/ISSUE_TEMPLATE .github/PULL_REQUEST_TEMPLATE
rm -f CHANGELOG.md CODE_OF_CONDUCT.md CONTRIBUTING.md

# Keep LICENSE but we'll modify it
# Keep README.md but we'll rewrite it

# Ensure we're on master branch
git checkout master 2>/dev/null || git checkout -b master

# Stage deletions
git add -A
git commit -m "chore: remove GitHub templates and docs for rebranding"
```

**Why keep window_manager as base?**

- ✅ Preserves fork relationship with upstream (can fetch updates)
- ✅ Keeps all native build files (CMakeLists.txt, etc.)
- ✅ Maintains git history for attribution
- ✅ We MODIFY the existing code, not rewrite from scratch

#### ✅ Step 5: Merge window_manager Code (COMPLETED)

**You already have tray_manager as base, now merge window_manager:**

```bash
# Create working branch from master (which has tray_manager code)
git checkout -b unified-tray-window

# Merge window_manager into our tray_manager base
# This brings in all window_manager code + history
git merge window/main --allow-unrelated-histories -m "feat: merge window_manager code

Merge window_manager from leanflutter/window_manager
- Source: https://github.com/leanflutter/window_manager
- License: MIT
- Author: leanflutter

This combines tray_manager and window_manager codebases
for the unified desktop_shell plugin."
```

**Result:**

- You now have BOTH codebases in one repo
- `lib/` has code from both projects
- `windows/`, `linux/`, `macos/` have implementations from both
- Full git history from both projects preserved
- Fork relationship with tray_manager (origin) intact

---

#### ✅ Step 6: Update LICENSE and README (COMPLETED)

**Create unified LICENSE:**

```bash
cat > LICENSE << 'EOF'
MIT License

Copyright (c) 2022-2024 LiJianying <lijy91@foxmail.com> (tray_manager and window_manager)
Copyright (c) 2025 aimer63 (desktop_shell modifications and unification)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

ATTRIBUTION:

This software includes code from:
- tray_manager (https://github.com/leanflutter/tray_manager) by leanflutter (LiJianying)
- window_manager (https://github.com/leanflutter/window_manager) by leanflutter (LiJianying)

Both original projects are MIT licensed.
EOF

git add LICENSE
git commit -m "docs: update LICENSE with attribution to both original projects"
```

**Update README.md:**

```bash
cat > README.md << 'EOF'
# desktop_shell

A unified Flutter desktop plugin combining system tray and window management with explicit error handling.

## Attribution

This project is built upon two excellent MIT-licensed projects:

- **[tray_manager](https://github.com/leanflutter/tray_manager)** by leanflutter - System tray functionality
- **[window_manager](https://github.com/leanflutter/window_manager)** by leanflutter - Window management

Both original projects are MIT licensed. See [LICENSE](LICENSE) for full attribution details.

## Features

- System tray icon with context menu
- Window show/hide/focus operations
- Minimize-to-tray behavior
- Explicit Result-based error handling (no exceptions)
- Windows, Linux, and macOS support

## Installation

```yaml
dependencies:
  desktop_shell:
    git:
      url: https://github.com/YOUR_USERNAME/desktop_shell.git
      ref: master
```

**License:**

MIT License. See [LICENSE](LICENSE) for details.
EOF

#### Step 7: Organize Merged Code

**After merging, reorganize files into unified structure:**

```bash
# Create unified source structure
mkdir -p lib/src/{tray,window}

# Move tray_manager files to tray/ subdirectory
git mv lib/src/tray_manager.dart lib/src/tray/ 2>/dev/null || true
git mv lib/src/tray_listener.dart lib/src/tray/ 2>/dev/null || true
# (other tray_manager files as needed)

# Keep window_manager files in window/ subdirectory
git mv lib/src/window_manager.dart lib/src/window/ 2>/dev/null || true

# Stage reorganization
git add -A
git commit -m "refactor: reorganize merged code into unified structure

- tray_manager code moved to lib/src/tray/
- window_manager code moved to lib/src/window/
- Preparation for unified API"
```

**Alternative: Keep files where they are and create unified wrapper:**

If moving files causes too many conflicts, keep both:

- `lib/src/tray_manager.dart` (original tray_manager)
- `lib/src/window_manager.dart` (original window_manager)
- Create `lib/desktop_shell.dart` as unified wrapper (Phase 2)

---

#### Step 8: Update pubspec.yaml

**Modify existing pubspec.yaml:**

```bash
# Backup original
cp pubspec.yaml pubspec.yaml.window_manager

# Update with new metadata
cat > pubspec.yaml << 'EOF'
name: desktop_shell
description: Unified Flutter desktop plugin for tray and window management
version: 0.1.0
homepage: https://github.com/YOUR_USERNAME/desktop_shell

environment:
  sdk: '>=3.0.0 <4.0.0'
  flutter: '>=3.0.0'

dependencies:
  flutter:
    sdk: flutter
  ffi: ^2.0.0
  path: ^1.8.0

dev_dependencies:
  flutter_test:
    sdk: flutter
  flutter_lints: ^3.0.0

flutter:
  plugin:
    platforms:
      linux:
        pluginClass: DesktopShellPlugin
      macos:
        pluginClass: DesktopShellPlugin
      windows:
        pluginClass: DesktopShellPlugin

# Attribution
# This plugin includes code from:
# - tray_manager (https://github.com/leanflutter/tray_manager) by leanflutter
# - window_manager (https://github.com/leanflutter/window_manager) by leanflutter
# Both are MIT licensed.
EOF

git add pubspec.yaml
git commit -m "chore: update pubspec.yaml for desktop_shell rebranding"
```

---

#### Step 9: Push Unified Branch

```bash
# Push unified branch to your fork
git push -u origin unified-tray-window

# Or push to master if ready
git checkout master
git merge unified-tray-window
git push -u origin master
```

---

#### Step 10: Verify Attribution

Check that your repository:

1. ✅ Shows proper attribution in README.md
2. ✅ Has ATTRIBUTION.md with detailed credits
3. ✅ Has LICENSE with both copyright lines
4. ✅ Git history shows both window_manager and tray_manager origins
5. ✅ Fork relationship with window_manager intact (can fetch upstream)

**Test the fork:**

```bash
# Verify you can build
flutter pub get
flutter build linux  # or windows/macos
```

---

#### Summary of What We Did

1. **Forked both repos** on GitHub (public attribution)
2. **Cloned window_manager** as our base (preserves fork relationship)
3. **Merged tray_manager** into our working branch (full history preserved)
4. **Created attribution files** (ATTRIBUTION.md, LICENSE, README.md)
5. **Reorganized code** into unified structure
6. **Committed and pushed** with proper attribution

**Result:**

- ✅ Proper MIT license compliance
- ✅ Full attribution to original authors
- ✅ Fork relationship preserved (can fetch upstream updates)
- ✅ Git history from both projects intact
- ✅ Ready to refactor into unified API

### Phase 1: Rebrand & Setup (Day 1 Continued)

1. **Rename package**
   - Edit `pubspec.yaml`:

     ```yaml
     name: desktop_shell
     description: Unified Flutter desktop plugin for tray and window management
     ```

2. **Update Dart class names**
   - `TrayManager` → `DesktoShell` (unified API)
   - `WindowManager` → Methods integrated into `DesktoShell`

3. **Strip Unnecessary Code**
   - From tray_manager: Keep icon, menu, events
   - From window_manager: Keep only show/hide/focus/preventClose

4. **Verify Build**

   ```bash
   flutter pub get
   flutter build linux
   ```

### Phase 2: Integrate Window Manager (Day 2)

1. **Extract Essentials from window_manager**
   - Copy window show/hide/focus implementations
   - Copy close interception logic
   - Adapt to tray_manager's platform channel style

2. **Unify Platform Channel**
   - Single channel: `desktop_shell`
   - Methods:
     - `initialize` - Setup tray and window with config
     - `showWindow` - Show window from tray
     - `hideWindow` - Hide window to tray
     - `focusWindow` - Focus window
     - `quit` - Cleanup and exit

3. **Dart API Unification**
   - Create `DesktoShell` class with Result-based API
   - Hide internal tray/window classes
   - Simple, direct API with explicit error handling

### Phase 3: Fix Windows Issues (Days 3-4)

1. **Menu Theme Research**
   - Study Windows 10/11 theming APIs
   - Test `SetWindowTheme` approach
   - Alternative: Custom draw menu

2. **Implement Theme Support**

   ```cpp
   // Detect dark mode
   BOOL isDarkMode = FALSE;
   DwmGetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, 
                         &isDarkMode, sizeof(isDarkMode));
   
   // Apply theme
   if (isDarkMode) {
     SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
   }
   ```

3. **Fix Menu Dismissal**
   - Debug current `TrackPopupMenu` usage
   - Add proper focus handling
   - Test on Windows 10/11

4. **Testing**
   - Light theme
   - Dark theme
   - Multiple monitors
   - HiDPI displays

### Phase 4: Fix Linux Issues (Days 5-7)

1. **Research StatusNotifierItem**
   - Study KDE/GNOME implementations
   - DBus interface specification
   - Menu implementation

2. **Implement StatusNotifierItem**
   - DBus service registration
   - Icon registration
   - Menu via dbusmenu

3. **Build System Updates**

   ```cmake
   # Remove libappindicator
   # find_package(PkgConfig REQUIRED)
   # pkg_check_modules(APPINDICATOR REQUIRED appindicator3-0.1)
   
   # Add dbus
   find_package(PkgConfig REQUIRED)
   pkg_check_modules(DBUS REQUIRED dbus-1)
   pkg_check_modules(DBUSMENU REQUIRED dbusmenu-glib-0.4)
   ```

4. **Testing**
   - Ubuntu 22.04 (X11)
   - Ubuntu 24.04 (Wayland)
   - Fedora (GNOME)
   - KDE Plasma

### Phase 5: Polish & Release (Days 8-10)

1. **Error Handling**
   - Graceful fallbacks
   - Informative error messages
   - Debug logging

2. **Documentation**
   - API documentation
   - Platform-specific notes
   - Troubleshooting guide
   - Migration guide from tray_manager

3. **Example App**
   - Simple test application
   - Demonstrates all features
   - Platform-specific tests

4. **Testing Matrix**
   - Windows 10 (light/dark)
   - Windows 11 (light/dark)
   - Ubuntu 22.04/24.04
   - macOS (if available)

5. **Publish**
   - GitHub release
   - Pub.dev (optional)
   - Tag version

## Testing Strategy

### Automated Tests (if possible)

```dart
// Unit tests for Dart API
test('tray icon initializes', () async {
  final result = await initialize(...);
  expect(result, isA<Ok>());
  expect(result.unwrap(), isA<DesktoShell>());
});

// Integration tests
// (Limited for desktop plugins, mostly manual)
```

### Manual Test Checklist

**Windows:**

- [ ] Icon appears in system tray
- [ ] Icon is crisp (HiDPI)
- [ ] Menu matches Windows theme (light/dark)
- [ ] Menu dismisses on click-outside
- [ ] Menu items are clickable
- [ ] "Open" shows and focuses window
- [ ] "Quit" closes app cleanly
- [ ] Close button minimizes to tray
- [ ] Single instance works

**Linux:**

- [ ] No build warnings
- [ ] Icon appears in system tray
- [ ] Icon visible on GNOME (with extension)
- [ ] Icon visible on KDE
- [ ] Menu appears on click
- [ ] Menu items are clickable
- [ ] Wayland: window shows (if supported)
- [ ] Close button minimizes to tray
- [ ] Single instance works

**macOS:**

- [ ] Icon appears in menu bar
- [ ] Menu appears on click
- [ ] All features work

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
| ------ | ------------- | -------- | ------------ |
| Windows theme fix too complex | Medium | High | Use basic Win32 theming, document limitation |
| StatusNotifierItem not supported | Low | High | Fallback to system tray, document requirements |
| Time overrun | Medium | Medium | Cut scope (skip macOS, skip nice-to-haves) |
| Breaking changes in Flutter | Low | High | Pin Flutter version, test before upgrade |

## Decision Points

### Before Starting

1. **Confirm scope:** Do we need macOS support?
2. **Confirm timeline:** Is 1-2 weeks acceptable?
3. **Confirm approach:** Fork tray_manager vs custom implementation?

### During Development

1. **Day 3:** Windows fix complexity assessment
2. **Day 6:** Linux StatusNotifierItem feasibility
3. **Day 8:** Scope adjustment if behind schedule

### Alternative: If Fork Fails

**Custom Platform Channel Implementation**

- Write only what we need
- Simpler but more initial work
- Timeline: 2-3 weeks
- Risk: Higher initial effort, lower long-term maintenance

## Success Criteria

- [ ] Single plugin replaces both tray_manager and window_manager
- [ ] Windows menu respects system theme
- [ ] Windows menu dismisses on click-outside
- [ ] Linux builds without deprecation warnings
- [ ] All current Chans functionality preserved
- [ ] Clean, simple API
- [ ] Documentation complete
- [ ] Tests pass on all target platforms

## Next Steps

1. **Create fork repository** on GitHub
2. **Day 1-2:** Setup and basic integration
3. **Day 3-4:** Windows fixes
4. **Day 5-7:** Linux fixes
5. **Day 8-10:** Polish and release

**Ready to start?**
