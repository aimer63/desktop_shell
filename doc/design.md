# Desktop Shell Design Document

## Overview

Desktop shell combines system tray and window management functionality into a
unified Flutter desktop plugin with explicit error handling via Result types.

## Design Philosophy

**Goal:** Create a maintainable, correct plugin with minimal API surface and
explicit error handling.

**Key principles:**

- **Explicit over implicit** - All callbacks required, no hidden defaults
- **Simplified naming** - `show` not `showWindow`, single window implied
- **Result-based errors** - No exceptions, all errors as typed values
- **Minimal surface** - Only essential features, user decides behavior

## Architecture

### State Management Options

We evaluated three approaches for managing state:

#### Option 1: File-Scoped Variables (tray_manager style)

```cpp
// tray_manager style
static AppIndicator* indicator = nullptr;
static GtkWidget* menu = nullptr;
```

**Pros:**

- Simple
- Proven to work
- Less code

**Cons:**

- No encapsulation
- Hard to test
- State not tied to object lifecycle
- Multiple instances impossible (theoretical limitation)

#### Option 2: Truly Global Variables

```cpp
// In header: extern AppIndicator* indicator;
// In one .cc file: AppIndicator* indicator = nullptr;
```

**Pros:**

- Accessible from any file

**Cons:**

- Violates encapsulation
- Creates hidden dependencies
- Harder to track state
- Not recommended

#### Option 3: Instance-Based Storage (Chosen)

```cpp
struct _DesktopShellPlugin {
  GObject parent_instance;
  FlPluginRegistrar* registrar;
  FlMethodChannel* channel;
  DesktopShellTrayManager* tray_manager;
  DesktopShellWindowManager* window_manager;
};
```

**Pros:**

- State tied to object lifecycle
- Proper encapsulation
- Easier to test
- Clear ownership
- Follows GObject conventions

**Cons:**

- More verbose
- Requires understanding of GObject reference counting
- More code than file-scoped approach

### Why We Chose Option 3

Despite Flutter desktop being single-instance, we chose instance-based storage
because:

1. **Correctness** - State belongs to the object that owns it
2. **Maintainability** - Clear where state lives and who manages it
3. **Testability** - Can create/destroy plugin instances in tests
4. **Consistency** - Aligns with GObject type system
5. **Future-proofing** - If multi-window support ever comes, architecture
   supports it

## Component Design

### Main Plugin (`desktop_shell_plugin.cc`)

**Responsibilities:**

- Register with Flutter plugin system
- Create and manage tray and window managers
- Route method calls to appropriate component
- Handle cleanup on destruction

**Key design points:**

- Uses `g_object_ref()` when passing self to channel handler
- Stores component managers as instance fields
- Proper dispose method for cleanup

### Tray Manager (`tray/tray_manager.cc`)

**Responsibilities:**

- Manage system tray icon via AppIndicator
- Handle tray menu creation and callbacks
- Communicate menu item clicks back to Dart

**Key design points:**

- GObject type with proper lifecycle
- Stores channel pointer for callbacks
- Menu widgets created on demand
- Proper cleanup in dispose

### Window Manager (`window/window_manager.cc`)

**Responsibilities:**

- Control window visibility (show/hide/focus)
- Intercept window close events
- Manage "prevent close" state (user-controlled)

**Key design points:**

- Stores window pointer and signal handler ID
- Delete event handler for intercepting close
- Simple boolean state for prevent_close

## Windows Architecture

Windows uses C++ classes instead of GObject. Key differences from Linux:

### Window Handle Acquisition

Linux gets window during plugin registration; Windows defers to `initialize`
call:

```cpp
// Windows: Get HWND when Flutter view is ready
HWND hwnd = ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
```

### Message Handling

Windows uses `HandleWindowMessage` delegate instead of GTK signals:

```cpp
// Register with Flutter's window proc chain
plugin->window_proc_id_ = registrar->RegisterTopLevelWindowProcDelegate(
    [plugin_ptr](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
      return plugin_ptr->HandleWindowMessage(hwnd, message, wparam, lparam);
    });
```

Handles:

- `WM_CLOSE` - Intercept close button
- `WM_TRAYMESSAGE` - Tray icon clicks (custom message)
- `WM_COMMAND` - Menu item clicks (sequential IDs 1024+)

### Menu IDs

Sequential IDs starting at 1024 (not hashCode):

- Avoids 16-bit overflow (Win32 WM_COMMAND limitation)
- Prevents collision with system menu indices (< 1024)

## Lifecycle Management

### Plugin Creation

```
1. Flutter calls register_with_registrar()
2. Create plugin instance (g_object_new)
3. Create method channel
4. Set up method call handler (with g_object_ref)
5. Create tray manager instance
6. Create window manager instance
7. Release our reference (channel holds one)
```

### Plugin Destruction

```
1. Flutter destroys plugin
2. Dispose called
3. Destroy tray manager
4. Destroy window manager
5. Release channel reference
6. Release registrar reference
```

### Reference Counting Patterns (from window_manager)

**Storing the registrar:**

```cpp
// Line 1089 in window_manager
plugin->registrar = FL_PLUGIN_REGISTRAR(g_object_ref(registrar));
```

**Passing plugin to channel handler:**

```cpp
// Line 1133 in window_manager
fl_method_channel_set_method_call_handler(
    plugin->channel, method_call_cb,
    g_object_ref(plugin),      // Ref for channel
    g_object_unref);           // Cleanup callback
```

**Releasing initial reference:**

```cpp
// Line 1135 in window_manager
g_object_unref(plugin);  // Channel now holds the ref we passed
```

**Cleaning up in dispose:**

```cpp
// Line 941 in window_manager
g_clear_object(&self->css_provider);
```

## Method Call Flow

```
Dart -> Platform Channel -> desktop_shell_plugin.cc
                                    |
            +-----------------------+-----------------------+
            |                                               |
    Tray Methods (setTrayIcon, setTrayMenu)  Window Methods (show, hide)
            |                                               |
    tray_manager.cc                                window_manager.cc
            |                                               |
    libappindicator                                  GTK Window APIs
```

## Error Handling

**Dart API:** All operations return `Result<(), Error>`.

**Error types:** Specific error per operation with `Option<String> code`:

```dart
final class TrayIconError extends DesktopShellError {
  final String details;
  final Option<String> code;  // Optional error code
  const TrayIconError(this.details, {this.code = const None()});
}
```

**Native responses:** All methods return Map with `success`, `message`
(always "OK" on success), and optional `code`:

```cpp
// Success
{"success": true, "message": "OK"}

// Error
{"success": false, "message": "Missing iconPath", "code": "MISSING_ICONPATH"}
```

### Error Codes

| Code | Description | Platform |
| ---- | ----------- | -------- |
| `TRAY_NOT_INITIALIZED` | Tray manager not created | Both |
| `WINDOW_MANAGER_NOT_INITIALIZED` | Window manager not created | Both |
| `INVALID_ARGS` | Arguments null or wrong type | Both |
| `MISSING_ICONPATH` | iconPath field missing/invalid | Both |
| `MISSING_MENU` | menu field missing/invalid | Both |
| `MISSING_PREVENT_FLAG` | prevent field missing/invalid | Both |
| `SET_ICON_FAILED` | Failed to set tray icon | Both |
| `SET_MENU_FAILED` | Failed to set tray menu | Both |
| `SHOW_FAILED` | Failed to show window | Both |
| `HIDE_FAILED` | Failed to hide window | Both |
| `FOCUS_FAILED` | Failed to focus window | Both |
| `SET_PREVENT_CLOSE_FAILED` | Failed to set prevent close | Both |
| `GET_HWND_FAILED` | Failed to get window handle | Windows |
| `POPUP_FAILED` | Failed to show context menu | Windows |
| `EXCEPTION` | C++ exception caught | Windows |

## Platform Channel Protocol

### Method Names

**Tray Methods:**

| Method | Description |
| ------ | ----------- |
| `setTrayIcon` | Set tray icon from path |
| `setTrayMenu` | Set context menu items |
| `popUpTrayMenu` | Show context menu (Windows/macOS) |

**Window Methods:**

| Method | Description |
| ------ | ----------- |
| `show` | Show hidden window |
| `hide` | Hide window to tray |
| `focus` | Bring window to front |
| `setPreventClose` | Intercept close button |

**Lifecycle:**

| Method | Description |
| ------ | ----------- |
| `destroy` | Cleanup resources |

### Events (Native -> Dart)

All callbacks required in `initialize()`:

```dart
static Future<Result<DesktopShell, DesktopShellError>> initialize({
  required String trayIcon,
  required List<MenuItem> trayItems,
  required void Function(DesktopShell shell) onWindowClose,
  required void Function(DesktopShell shell) onTrayIconClick,
  required void Function(DesktopShell shell, MenuItem item)
    onTrayMenuItemClick,
}) async
```

| Event | Required | Description |
| ----- | -------- | ----------- |
| `onWindowClose` | Yes | User clicks close button (use to hide to tray) |
| `onTrayIconClick` | Yes | User clicks tray icon |
| `onTrayMenuItemClick` | Yes | User selects menu item |

**Note:** No per-item `onClick` on MenuItem. All menu clicks go through
`onTrayMenuItemClick`.

## Dart API

### Initialization

```dart
final result = await DesktopShell.initialize(
  trayIcon: 'assets/icon.png',
  trayItems: [
    MenuItem(key: 'show', label: 'Show Window'),
    MenuItem.separator(),
    MenuItem(key: 'quit', label: 'Quit'),
  ],
  onWindowClose: (shell) async => await shell.hideWindow(),
  onTrayIconClick: (shell) async {},  // Linux: no-op
  onTrayMenuItemClick: (shell, item) async {
    switch (item.key) {
      case 'show':
        await shell.showWindow();
        await shell.focusWindow();
      case 'quit':
        await shell.destroy();
        exit(0);
    }
  },
);
```

### DesktopShell Interface

```dart
abstract class DesktopShell {
  Future<Result<(), TrayIconError>> setTrayIcon(String iconPath);
  Future<Result<(), TrayMenuError>> setTrayMenu(List<MenuItem> items);
  Future<Result<(), TrayPopupError>> popUpTrayMenu();
  Future<Result<(), WindowShowError>> show();
  Future<Result<(), WindowHideError>> hide();
  Future<Result<(), WindowFocusError>> focus();
  Future<Result<(), WindowPreventCloseError>> setPreventClose(bool prevent);
  Future<Result<(), ShellDestroyError>> destroy();
}
```

**Key design decisions:**

- Simplified names: `show` not `showWindow` (single window implied)
- `setPreventClose` not called automatically - user decides when
- All methods return `Result<(), Error>` - explicit error handling

## MenuItem

```dart
final class MenuItem {
  final String key;        // Unique identifier
  final String label;      // Display text
  final bool checked;      // Checkbox state
  final bool isSeparator;  // True for separator

  const MenuItem({required this.key, required this.label, this.checked = false})
    : isSeparator = false;

  const MenuItem.separator()
    : key = '',
      label = '',
      checked = false,
      isSeparator = true;
}
```

**No per-item callback.** Menu clicks handled globally via
`onTrayMenuItemClick` in `initialize()`.

## Platform-Specific Behavior

### `popUpTrayMenu()`

| Platform | Mechanism | User Action |
| -------- | --------- | ----------- |
| **Linux** | Automatic | None - menu appears on click |
| **Windows** | `TrackPopupMenu(hMenu, x, y)` | Must call in `onTrayIconClick` |
| **macOS** | `performClick()` on statusItem | Must call in `onTrayIconClick` |

**Note:** On Linux this method returns `Ok(())` immediately without doing
anything.

### `setPreventClose()`

**Not called automatically in initialize().** User must explicitly call:

```dart
final result = await initialize(...);
if (result case Ok(value: final shell)) {
  await shell.setPreventClose(true);  // User decides
}
```

This flexibility supports:

- Tray-only apps (no window)
- Window-only apps (no tray)
- Mixed use cases

## Build Configuration

Both Linux and Windows use CMake. The plugin CMakeLists.txt files reference
functions and variables from the app's CMakeLists.txt.

### CMake Configuration

**CRITICAL:** The `apply_standard_settings()` function is called in the plugin
but defined in your **app's** `linux/CMakeLists.txt` (not this plugin's). It is
generated by `flutter create --platforms=linux`. If you manually created the
CMake files, you must either recreate the app with Flutter or manually copy
the function from the Flutter SDK template.

**Linux example:**

```cmake
# Use Flutter's standard settings (defined in app's linux/CMakeLists.txt)
apply_standard_settings(${PLUGIN_NAME})

# Disable deprecated warnings for appindicator
target_compile_options(${PLUGIN_NAME} PRIVATE
  -Wno-deprecated-declarations
)

# Find appindicator
pkg_check_modules(APPINDICATOR IMPORTED_TARGET ayatana-appindicator3-0.1)
if(NOT APPINDICATOR_FOUND)
  pkg_check_modules(APPINDICATOR IMPORTED_TARGET appindicator3-0.1)
endif()

# Link libraries
target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  PkgConfig::APPINDICATOR
  PkgConfig::GTK
)
```

### Linux (CMake)

Uses CMake with `apply_standard_settings` from Flutter SDK template.

**Plugin structure:**

```
linux/
├── CMakeLists.txt                  # Build configuration
├── desktop_shell_plugin.cc         # Main plugin (GObject, method routing)
├── desktop_shell_plugin.h          # Plugin header
├── include/
│   └── desktop_shell/
│       └── desktop_shell_plugin.h  # Public C header
├── tray/
│   ├── tray_manager.cc             # AppIndicator implementation
│   └── tray_manager.h
└── window/
    ├── window_manager.cc           # GTK window implementation
    └── window_manager.h
```

**Requirements:**

- `libappindicator3-dev` or `libayatana-appindicator3-dev`
- `pkg-config` for library detection
- GTK3 development files

### Windows (CMake)

Uses standard Flutter Windows plugin template with CMake.

**Plugin structure:**

```
windows/
├── desktop_shell_plugin.cpp    # Main plugin
│   #   (HandleMethodCall, HandleWindowMessage)
├── desktop_shell_plugin.h      # C export header
├── include/
│   └── desktop_shell/
│       └── desktop_shell_plugin.h  # Public C header
├── tray/
│   ├── tray_icon.cpp           # Shell_NotifyIcon implementation
│   └── tray_icon.h
└── window/
    ├── window_manager.cpp      # ShowWindow, SetForegroundWindow
    └── window_manager.h
```

**Requirements:**

- Windows SDK
- Visual Studio 2019 or later (or Build Tools)

**Build settings:**

- Links against standard Windows libraries (shell32, user32, gdi32)
- Uses Common Controls v6 for modern menu theming (manifest)
- Standard C++17

## Testing Strategy

### Unit Testing

Difficult for native code due to GTK/AppIndicator dependencies. Options:

- Mocking GTK functions
- Integration tests via example app
- Manual testing on target platforms

### Integration Testing

Use example app to verify:

1. Tray icon appears
2. Menu items work
3. Window show/hide/focus work
4. Close interception works (if setPreventClose called)
5. Cleanup works (no crashes on exit)

## Known Limitations

1. **Single tray** - One tray icon per app instance
2. **AppIndicator deprecated** - Future: migrate to StatusNotifierItem
3. **No multi-monitor awareness** - Tray appears on primary

## References

- tray_manager: `/home/aimer/devel/flutter/tray_manager`
- window_manager: `/home/aimer/devel/flutter/window_manager`
- GObject documentation: <https://docs.gtk.org/gobject/>
- Flutter Linux plugins: <https://docs.flutter.dev/desktop#linux>
- Win32 Shell API: <https://learn.microsoft.com/en-us/windows/win32/api/shellapi/>
- Flutter Windows plugins: <https://docs.flutter.dev/desktop#windows>
