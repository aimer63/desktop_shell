# Desktop Shell Implementation Plan

## Goal

Implement desktop_shell using **Option 3: Instance-based storage** with proper
GObject types, following the patterns from window_manager, with unified tray
and window management, explicit error handling, and minimal API surface.

## Architecture

```
lib/
├── desktop_shell.dart              # Main export
└── src/
    ├── api.dart                    # DesktopShell interface + initialize()
    ├── errors.dart                 # Error types with Option<String> code
    └── menu/
        └── menu.dart               # Menu/MenuItem classes

linux/
├── CMakeLists.txt                  # Build configuration
├── desktop_shell_plugin.cc         # Main plugin (registrar)
├── desktop_shell_plugin.h          # Plugin header
├── include/
│   └── desktop_shell/
│       └── desktop_shell_plugin.h  # Public header
├── tray/
│   ├── tray_manager.cc             # Tray implementation
│   └── tray_manager.h              # Tray interface
└── window/
    ├── window_manager.cc           # Window implementation
    └── window_manager.h            # Window interface
```

## Step-by-Step Implementation

### Step 1: Clean Up Current Implementation

**Files to review/modify:**

- `linux/CMakeLists.txt` - Fix to use `apply_standard_settings`
- `linux/desktop_shell_plugin.cc` - Ensure proper lifecycle
- `linux/tray/tray_manager.cc` - Verify GObject type registration
- `linux/window/window_manager.cc` - Verify GObject type registration

**Verification:**

- Check all files compile
- Verify no deprecated function warnings treated as errors

### Step 2: Verify Plugin Structure

**Main plugin (`desktop_shell_plugin.cc`) must:**

1. Define plugin struct with tray and window manager instances:

   ```cpp
   struct _DesktopShellPlugin {
     GObject parent_instance;
     FlPluginRegistrar* registrar;
     FlMethodChannel* channel;
     DesktopShellTrayManager* tray_manager;
     DesktopShellWindowManager* window_manager;
   };
   ```

2. Proper reference counting in registration:

   ```cpp
   fl_method_channel_set_method_call_handler(
       plugin->channel, method_call_cb,
       g_object_ref(plugin), g_object_unref);
   ```

3. Dispose method cleanup:

   ```cpp
   g_clear_object(&self->tray_manager);
   g_clear_object(&self->window_manager);
   g_clear_object(&self->channel);
   g_clear_object(&self->registrar);
   ```

### Step 3: CMakeLists.txt Configuration

**Where `apply_standard_settings` is defined:**

**File:** `flutter_tools/templates/app/linux.tmpl/CMakeLists.txt.tmpl`
**Lines:** 42-47

```cmake
function(APPLY_STANDARD_SETTINGS TARGET)
  target_compile_features(${TARGET} PUBLIC cxx_std_14)
  target_compile_options(${TARGET} PRIVATE -Wall -Werror)
  target_compile_options(${TARGET} PRIVATE "$<$<NOT:$<CONFIG:Debug>>:-O3>")
  target_compile_definitions(${TARGET} PRIVATE
    "$<$<NOT:$<CONFIG:Debug>>:NDEBUG>")
endfunction()
```

**What it does:**

- Sets C++14 standard
- Enables all warnings (`-Wall`)
- Treats warnings as errors (`-Werror`)
- Sets optimization to O3 for release builds
- Defines NDEBUG for release builds

**Availability:**

- Function is defined in app's `linux/CMakeLists.txt`
- Plugin CMakeLists.txt is subdirectory'd from app
- Function available in parent scope
- Standard Flutter plugin contract

**Important:** This function only exists if the app was created with
`flutter create --platforms=linux`. Manually created CMake files will lack it.

**Plugin CMakeLists.txt:**

```cmake
cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "desktop_shell")
project(${PROJECT_NAME} LANGUAGES CXX)

# Flutter requires C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLUGIN_NAME "${PROJECT_NAME}_plugin")

add_library(${PLUGIN_NAME} SHARED
  "desktop_shell_plugin.cc"
  "tray/tray_manager.cc"
  "window/window_manager.cc"
)

# Use Flutter's standard settings
apply_standard_settings(${PLUGIN_NAME})

set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(${PLUGIN_NAME} PRIVATE flutter)
target_link_libraries(${PLUGIN_NAME} PRIVATE PkgConfig::GTK)

# AppIndicator
find_package(PkgConfig REQUIRED)
pkg_check_modules(APPINDICATOR IMPORTED_TARGET ayatana-appindicator3-0.1)
if(NOT APPINDICATOR_FOUND)
  pkg_check_modules(APPINDICATOR IMPORTED_TARGET appindicator3-0.1)
endif()

if(NOT APPINDICATOR_FOUND)
  message(FATAL_ERROR "libappindicator3 required")
endif()

# Link libraries
target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  PkgConfig::APPINDICATOR
  PkgConfig::GTK
)

# Disable deprecated warnings for appindicator
target_compile_options(${PLUGIN_NAME} PRIVATE
  -Wno-deprecated-declarations
)
```

### Step 4: Tray Manager Implementation

**File:** `linux/tray/tray_manager.cc`

**Requirements:**

1. **GObject type definition:**

   ```cpp
   struct _DesktopShellTrayManager {
     GObject parent_instance;
     FlMethodChannel* channel;
     AppIndicator* indicator;
     GtkWidget* menu;
   };

   G_DEFINE_TYPE(DesktopShellTrayManager, desktop_shell_tray_manager,
                 G_TYPE_OBJECT)
   ```

2. **Constructor takes channel:**

   ```cpp
   DesktopShellTrayManager* desktop_shell_tray_manager_new(
       FlMethodChannel* channel)
   ```

3. **Dispose cleanup:**

   ```cpp
   if (self->indicator) {
     app_indicator_set_status(self->indicator,
                              APP_INDICATOR_STATUS_PASSIVE);
     g_clear_object(&self->indicator);
   }
   if (self->menu) {
     gtk_widget_destroy(self->menu);
     self->menu = nullptr;
   }
   g_clear_object(&self->channel);
   ```

4. **Methods:**
   - `set_icon()` - Create indicator if needed, set icon
   - `set_menu()` - Build menu from Flutter data, attach to indicator

### Step 5: Window Manager Implementation

**File:** `linux/window/window_manager.cc`

**Requirements:**

1. **GObject type definition:**

   ```cpp
   struct _DesktopShellWindowManager {
     GObject parent_instance;
     GtkWindow* window;
     gboolean prevent_close;
     gulong delete_handler_id;
   };

   typedef struct {
     GObjectClass parent_class;
   } DesktopShellWindowManagerClass;

   G_DEFINE_TYPE(DesktopShellWindowManager, desktop_shell_window_manager,
                 G_TYPE_OBJECT)
   ```

2. **Constructor takes window:**

   ```cpp
   DesktopShellWindowManager* desktop_shell_window_manager_new(
       GtkWindow* window)
   ```

3. **Connect delete event handler:**

   ```cpp
   self->delete_handler_id = g_signal_connect(
       window, "delete-event",
       G_CALLBACK(on_delete_event), self);
   ```

4. **Delete event callback:**

   ```cpp
   static gboolean on_delete_event(GtkWidget* widget,
                                   GdkEvent* event,
                                   gpointer user_data) {
     DesktopShellWindowManager* self = user_data;
     if (self->prevent_close) {
       gtk_widget_hide(GTK_WIDGET(self->window));
       return TRUE;  // Prevent close
     }
     return FALSE;  // Allow close
   }
   ```

5. **Methods:**
   - `show()` - `gtk_widget_show()` + `gtk_window_present()`
   - `hide()` - `gtk_widget_hide()`
   - `focus()` - `gtk_window_present()`
   - `set_prevent_close()` - Set boolean flag

### Step 6: Method Call Handler

**In `desktop_shell_plugin.cc`:**

**Critical:** All native handlers must return a Map with 'success' field:

```cpp
// Return Map with success status and optional message/code
static FlMethodResponse* handle_set_tray_icon(DesktopShellPlugin* self,
                                              FlValue* args) {
  g_autoptr(FlValue) response = fl_value_new_map();
  
  FlValue* icon_path_value = fl_value_lookup_string(args, "iconPath");
  if (icon_path_value == nullptr) {
    fl_value_set_string(response, "success", fl_value_new_bool(false));
    fl_value_set_string(response, "code", fl_value_new_string("INVALID_ARGS"));
    fl_value_set_string(response, "message",
                        fl_value_new_string("Missing iconPath"));
    return FL_METHOD_RESPONSE(
        fl_method_success_response_new(response));
  }
  
  const gchar* icon_path = fl_value_get_string(icon_path_value);
  
  if (!desktop_shell_tray_manager_set_icon(self->tray_manager, icon_path)) {
    fl_value_set_string(response, "success", fl_value_new_bool(false));
    fl_value_set_string(response, "code", fl_value_new_string("SET_FAILED"));
    fl_value_set_string(response, "message",
                        fl_value_new_string("Failed to set tray icon"));
    return FL_METHOD_RESPONSE(
        fl_method_success_response_new(response));
  }
  
  // Success
  fl_value_set_string(response, "success", fl_value_new_bool(true));
  fl_value_set_string(response, "message",
                      fl_value_new_string("Icon set successfully"));
  return FL_METHOD_RESPONSE(
      fl_method_success_response_new(response));
}

// NEVER return null or implicit void
// WRONG: return nullptr;  // DON'T DO THIS
// WRONG: missing return;  // DON'T DO THIS
```

**Method name mapping (Dart -> Native):**

| Dart Method | Native Method |
| ----------- | ------------- |
| `show()` | `show` |
| `hide()` | `hide` |
| `focus()` | `focus` |
| `popUpTrayMenu()` | `popUpTrayMenu` |
| `setTrayIcon()` | `setTrayIcon` |
| `setTrayMenu()` | `setTrayMenu` |
| `setPreventClose()` | `setPreventClose` |
| `destroy()` | `destroy` |

Implement handler for each method:

```cpp
static void method_call_cb(FlMethodChannel* channel,
                           FlMethodCall* method_call,
                           gpointer user_data) {
  DesktopShellPlugin* self = DESKTOP_SHELL_PLUGIN(user_data);
  const gchar* method = fl_method_call_get_name(method_call);
  FlValue* args = fl_method_call_get_args(method_call);

  g_autoptr(FlMethodResponse) response = nullptr;

  if (strcmp(method, "setTrayIcon") == 0) {
    response = handle_set_tray_icon(self, args);
  } else if (strcmp(method, "setTrayMenu") == 0) {
    response = handle_set_tray_menu(self, args);
  } else if (strcmp(method, "popUpTrayMenu") == 0) {
    response = handle_pop_up_tray_menu(self);
  } else if (strcmp(method, "show") == 0) {
    response = handle_show(self);
  } else if (strcmp(method, "hide") == 0) {
    response = handle_hide(self);
  } else if (strcmp(method, "focus") == 0) {
    response = handle_focus(self);
  } else if (strcmp(method, "setPreventClose") == 0) {
    response = handle_set_prevent_close(self, args);
  } else if (strcmp(method, "destroy") == 0) {
    response = handle_destroy(self);
  } else {
    response = FL_METHOD_RESPONSE(
        fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}
```

### Step 7: Dart API with Strict Types

**File:** `lib/src/api.dart`

**Direct SDK calls with explicit types - no wrappers:**

```dart
import 'dart:async';
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;
import 'package:unwrap_me/unwrap_me.dart';

import 'errors.dart';
import 'menu/menu.dart';

const _channel = MethodChannel('desktop_shell');

/// Initialize desktop shell with tray and window management.
///
/// All callbacks are required - no hidden defaults.
/// setPreventClose is NOT called automatically - user decides when.
Future<Result<DesktopShell, DesktopShellError>> initialize({
  required String trayIcon,
  required List<MenuItem> trayItems,
  required void Function(DesktopShell shell) onWindowClose,
  required void Function(DesktopShell shell) onTrayIconClick,
  required void Function(DesktopShell shell, MenuItem item) onTrayMenuItemClick,
}) async {
  // Check platform support
  if (!Platform.isWindows && !Platform.isLinux && !Platform.isMacOS) {
    return const Err(UnsupportedPlatformError('unsupported'));
  }

  // Create shell instance
  final shell = _DesktopShellImpl(
    onWindowClose: onWindowClose,
    onTrayIconClick: onTrayIconClick,
    onTrayMenuItemClick: onTrayMenuItemClick,
  );

  // Register handler for native events
  _channel.setMethodCallHandler((call) async {
    await shell._dispatchNativeEvent(call);
  });

  // Initialize tray icon
  final iconResult = await shell.setTrayIcon(trayIcon);
  if (iconResult case Err(:final error)) {
    return Err(error);
  }

  // Initialize tray menu
  final menuResult = await shell.setTrayMenu(trayItems);
  if (menuResult case Err(:final error)) {
    return Err(error);
  }

  return Ok(shell);
}

/// Main desktop shell controller interface.
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

1. **Simplified names:** `show` not `showWindow` (single window implied)
2. **Required callbacks:** All three callbacks required, no nullable
3. **No forced setPreventClose:** User must call explicitly if needed
4. **Direct SDK calls:** No wrapper functions, direct invokeMethod
5. **Map-based responses:** Native returns Map with success/message/code

**Event handling (Native -> Dart):**

```dart
Future<void> _dispatchNativeEvent(MethodCall call) async {
  switch (call.method) {
    case 'onWindowClose':
      onWindowClose(this);
    case 'onTrayIconClick':
      onTrayIconClick(this);
    case 'onTrayMenuItemClick':
      final key = call.arguments['key'] as String;
      final item = _findMenuItemByKey(_currentMenu!.items, key);
      if (item != null) {
        onTrayMenuItemClick(this, item);
      }
  }
}
```

### Step 8: Error Types

**File:** `lib/src/errors.dart`

```dart
import 'package:unwrap_me/unwrap_me.dart';

/// Base error for all desktop_shell errors.
sealed class DesktopShellError {
  const DesktopShellError();
  String get message;
}

final class TrayIconError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const TrayIconError(this.details, {this.code = const None()});
  @override
  String get message => 'Tray icon error: $details';
}

final class TrayMenuError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const TrayMenuError(this.details, {this.code = const None()});
  @override
  String get message => 'Tray menu error: $details';
}

final class TrayPopupError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const TrayPopupError(this.details, {this.code = const None()});
  @override
  String get message => 'Tray popup error: $details';
}

final class WindowShowError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const WindowShowError(this.details, {this.code = const None()});
  @override
  String get message => 'Window show error: $details';
}

final class WindowHideError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const WindowHideError(this.details, {this.code = const None()});
  @override
  String get message => 'Window hide error: $details';
}

final class WindowFocusError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const WindowFocusError(this.details, {this.code = const None()});
  @override
  String get message => 'Window focus error: $details';
}

final class WindowPreventCloseError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const WindowPreventCloseError(this.details, {this.code = const None()});
  @override
  String get message => 'Prevent close error: $details';
}

final class ShellDestroyError extends DesktopShellError {
  final String details;
  final Option<String> code;
  const ShellDestroyError(this.details, {this.code = const None()});
  @override
  String get message => 'Destroy error: $details';
}

final class TrayInitError extends DesktopShellError {
  final String details;
  const TrayInitError(this.details);
  @override
  String get message => 'Tray init error: $details';
}

final class WindowInitError extends DesktopShellError {
  final String details;
  const WindowInitError(this.details);
  @override
  String get message => 'Window init error: $details';
}

final class UnsupportedPlatformError extends DesktopShellError {
  final String details;
  const UnsupportedPlatformError(this.details);
  @override
  String get message => 'Unsupported platform: $details';
}
```

### Step 9: Menu Types

**File:** `lib/src/menu/menu.dart`

```dart
/// A menu item for the system tray context menu.
final class MenuItem {
  final String key;
  final String label;
  final bool isSeparator;
  final bool checked;

  const MenuItem({
    required this.key,
    required this.label,
    this.checked = false,
  }) : isSeparator = false;

  const MenuItem.separator()
      : key = '',
        label = '',
        isSeparator = true,
        checked = false;

  Map<String, dynamic> toJson() => {
        'id': key.hashCode,
        'key': key,
        'label': label,
        'type': isSeparator ? 'separator' : (checked ? 'checkbox' : 'normal'),
        'checked': checked,
      };
}

/// A menu containing multiple menu items.
final class Menu {
  final List<MenuItem> items;

  const Menu({required this.items});

  List<Map<String, dynamic>> toJson() =>
      items.map((item) => item.toJson()).toList();

  MenuItem? getMenuItemById(int id) {
    for (final item in items) {
      if (item.key.hashCode == id) return item;
    }
    return null;
  }
}
```

**No per-item onClick.** All menu clicks handled by global `onTrayMenuItemClick`
in `initialize()`.

### Step 10: Example App

**File:** `example/lib/main.dart`

**Requirements:**

- Initialize shell in main()
- Handle window close by hiding to tray
- Tray menu items: Show Window, Quit
- UI buttons: Show, Hide, Focus, Quit
- Call setPreventClose explicitly after initialization

**Example:**

```dart
import 'dart:io';
import 'package:desktop_shell/desktop_shell.dart';
import 'package:unwrap_me/unwrap_me.dart';

void main() async {
  final result = await initialize(
    trayIcon: 'assets/icon.png',
    trayItems: [
      MenuItem(key: 'show', label: 'Show Window'),
      MenuItem.separator(),
      MenuItem(key: 'quit', label: 'Quit'),
    ],
    onWindowClose: (shell) async {
      // Hide to tray instead of closing
      await shell.hide();
    },
    onTrayIconClick: (shell) async {
      // Linux: menu appears automatically
      // Windows/macOS: call popUpTrayMenu here
    },
    onTrayMenuItemClick: (shell, item) async {
      switch (item.key) {
        case 'show':
          await shell.show();
          await shell.focus();
        case 'quit':
          await shell.destroy();
          exit(0);
      }
    },
  );

  switch (result) {
    case Ok(value: final shell):
      // User decides when to prevent close
      await shell.setPreventClose(true);
      
      runApp(MyApp(shell: shell));
    case Err(:final error):
      stderr.writeln('Failed to initialize: ${error.message}');
      exit(1);
  }
}
```

### Step 11: Build and Test

**Commands:**

```bash
cd example
flutter clean
flutter pub get
flutter run -d linux
```

**Verification checklist:**

- [ ] Builds without errors
- [ ] Tray icon appears
- [ ] Tray menu shows on click
- [ ] "Show Window" works from menu
- [ ] "Quit" works from menu
- [ ] Hide to Tray button works
- [ ] Show Window button works
- [ ] Focus Window button works
- [ ] Window close button hides to tray (after setPreventClose)
- [ ] No crashes on exit

## Common Issues and Solutions

### Issue: `apply_standard_settings` not found

**Cause:** App not created with `flutter create --platforms=linux`

**Solution:** Use `flutter create` to generate proper CMakeLists.txt

### Issue: Deprecated function warnings as errors

**Cause:** `-Werror` flag from `apply_standard_settings`

**Solution:** Add `-Wno-deprecated-declarations` to CMakeLists.txt

### Issue: Plugin crashes on startup

**Cause:** Incorrect reference counting

**Solution:** Ensure `g_object_ref(plugin)` passed to channel handler

### Issue: Menu not appearing

**Cause:** Menu widgets not shown before attaching

**Solution:** Call `gtk_widget_show(menu)` before
`app_indicator_set_menu()`

### Issue: Window close not intercepted

**Cause:** Delete event handler not connected or returning wrong value

**Solution:** Connect in constructor, return TRUE to prevent close

## Platform-Specific Behavior

### `popUpTrayMenu()` - Menu Display

**Platform Differences:**

| Platform | Mechanism | User Action Required |
| -------- | ----------- | -------------------- |
| **Linux** | Automatic | None - menu appears on icon click |
| **Windows** | `TrackPopupMenu(hMenu, x, y)` | Must call in `onTrayIconClick` handler |
| **macOS** | `performClick()` on statusItem | Must call in `onTrayIconClick` handler |

**Why Different:**

- **Linux (AppIndicator):** System handles click, automatically shows menu set
  via `setTrayMenu()`. No API exists to programmatically trigger menu.

- **Windows:** Requires explicit `TrackPopupMenu()` call with coordinates.
  Windows API: `TrackPopupMenu(hMenu, flags, x, y, reserved, hWnd, rect)`

- **macOS:** Requires `performClick()` on statusItem button. Indirect - tells
  system "simulate user click", system then shows attached menu.

**Implementation Strategy:**

```dart
// Same API on all platforms
Future<Result<(), TrayError>> popUpTrayMenu();

// Linux: returns Ok(()) immediately (no-op)
// Windows: calls TrackPopupMenu
// macOS: calls performClick
```

**User Code Pattern:**

```dart
void onTrayIconClick() async {
  // Call on all platforms - Linux returns immediately (no-op)
  await shell.popUpTrayMenu();
}
```

### Summary

All platforms expose identical Dart API, but internal implementation differs:

- Linux: System-controlled menu display
- Windows/macOS: Programmatic menu display

## References

- window_manager: `/home/aimer/devel/flutter/window_manager`
- tray_manager: `/home/aimer/devel/flutter/tray_manager`
- GObject docs: <https://docs.gtk.org/gobject/>
- Flutter Linux plugins: <https://docs.flutter.dev/desktop#linux>
