# Desktop Shell Implementation Plan

## Goal

Implement desktop_shell using **Option 3: Instance-based storage** with proper
GObject types, following the patterns from window_manager.

## Architecture

```
lib/
├── desktop_shell.dart              # Main export
└── src/
    ├── api.dart                    # DesktopShell interface
    ├── errors.dart                 # Error types
    ├── platform_channel.dart       # Method channel wrapper
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

**Must include:**

```cmake
# Use Flutter's standard settings
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

if(NOT APPINDICATOR_FOUND)
  message(FATAL_ERROR "libappindicator3 required")
endif()

# Link libraries
target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  PkgConfig::APPINDICATOR
  PkgConfig::GTK
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
  } else if (strcmp(method, "showWindow") == 0) {
    response = handle_show_window(self);
  } else if (strcmp(method, "hideWindow") == 0) {
    response = handle_hide_window(self);
  } else if (strcmp(method, "focusWindow") == 0) {
    response = handle_focus_window(self);
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
import 'package:unwrap_me/unwrap_me.dart';
import 'package:flutter/services.dart';

/// The single platform channel for all operations.
const _channel = MethodChannel('desktop_shell');

/// Set tray icon. Returns Ok(()) on success, Err on failure.
Future<Result<(), TrayIconError>> setTrayIcon(String iconPath) async {
  try {
    final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
      'setTrayIcon',
      {'iconPath': iconPath},
    );

    if (result == null) {
      return const Err(TrayIconError('Native returned null'));
    }

    if (result['success'] == true) {
      return const Ok(());
    }

    return Err(TrayIconError(
      result['message'] as String? ?? 'Unknown error',
      code: Option.fromNullable(result['code'] as String?),
    ));
  } catch (e) {
    return Err(TrayIconError(e.toString()));
  }
}

/// Show window. Returns Ok(()) on success, Err on failure.
Future<Result<(), WindowShowError>> showWindow() async {
  try {
    final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
      'showWindow',
    );

    if (result == null) {
      return const Err(WindowShowError('Native returned null'));
    }

    if (result['success'] == true) {
      return const Ok(());
    }

    return Err(WindowShowError(
      result['message'] as String? ?? 'Unknown error',
      code: Option.fromNullable(result['code'] as String?),
    ));
  } catch (e) {
    return Err(WindowShowError(e.toString()));
  }
}

/// Set tray context menu.
Future<Result<(), TrayMenuError>> setTrayMenu(List<MenuItem> items) async {
  try {
    final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
      'setTrayMenu',
      {
        'menu': items.map((i) => i.toJson()).toList(),
      },
    );

    if (result == null) {
      return const Err(TrayMenuError('Native returned null'));
    }

    if (result['success'] == true) {
      return const Ok(());
    }

    return Err(TrayMenuError(
      result['message'] as String? ?? 'Unknown error',
      code: Option.fromNullable(result['code'] as String?),
    ));
  } catch (e) {
    return Err(TrayMenuError(e.toString()));
  }
}

/// Show context menu (Linux: no-op, automatic on click).
Future<Result<(), TrayError>> popUpContextMenu() async {
  try {
    final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
      'popUpContextMenu',
    );

    if (result == null) {
      return const Err(TrayError('Native returned null'));
    }

    if (result['success'] == true) {
      return const Ok(());
    }

    return Err(TrayError(
      result['message'] as String? ?? 'Unknown error',
      code: Option.fromNullable(result['code'] as String?),
    ));
  } catch (e) {
    return Err(TrayError(e.toString()));
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

/// Tray-related errors.
sealed class TrayError extends DesktopShellError {
  const TrayError();
}

final class TrayIconError extends TrayError {
  final String details;
  final Option<String> code;

  const TrayIconError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to set tray icon: $details';
}

final class TrayMenuError extends TrayError {
  final String details;
  final Option<String> code;

  const TrayMenuError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to set tray menu: $details';
}

/// Window operation errors.
sealed class WindowOperationError extends DesktopShellError {
  const WindowOperationError();
}

final class WindowShowError extends WindowOperationError {
  final String details;
  final Option<String> code;

  const WindowShowError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to show window: $details';
}

final class WindowHideError extends WindowOperationError {
  final String details;
  final Option<String> code;

  const WindowHideError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to hide window: $details';
}

final class WindowInitError extends WindowOperationError {
  final String details;
  final Option<String> code;

  const WindowInitError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Window initialization failed: $details';
}
```

**File:** `lib/src/api.dart`

```dart
import 'package:unwrap_me/unwrap_me.dart';

/// Main desktop shell controller.
///
/// All operations return [Result] for explicit error handling.
abstract class DesktopShell {
  /// Set tray icon from path.
  /// Returns [Ok(())] on success, [Err(TrayIconError)] on failure.
  Future<Result<(), TrayIconError>> setTrayIcon(String iconPath);
  
  /// Set tray context menu.
  Future<Result<(), TrayMenuError>> setTrayMenu(List<MenuItem> items);
  
  /// Show context menu (Linux: no-op, automatic on click).
  Future<Result<(), TrayError>> popUpContextMenu();
  
  /// Show window from tray.
  Future<Result<(), WindowShowError>> showWindow();
  
  /// Hide window to tray.
  Future<Result<(), WindowHideError>> hideWindow();
  
  /// Focus window.
  Future<Result<(), WindowOperationError>> focusWindow();
  
  /// Set prevent close flag.
  Future<Result<(), WindowOperationError>> setPreventClose(bool prevent);

  /// Cleanup and destroy resources.
  Future<Result<(), DesktopShellError>> destroy();
}

### Events (Callbacks)

**File:** `lib/src/api.dart`

Events are handled via callbacks passed to `initialize()`:

```dart
Future<Result<DesktopShell, DesktopShellError>> initialize({
  required String trayIcon,
  required List<MenuItem> trayItems,
  required void Function(DesktopShell shell) onWindowClose,
  void Function(DesktopShell shell)? onTrayIconClick,
  void Function(DesktopShell shell, MenuItem item)? onTrayMenuItemClick,
}) async {
  // ... setup ...
  
  // Set up method call handler for events from native
  _channel.setMethodCallHandler((call) async {
    switch (call.method) {
      case 'onWindowClose':
        onWindowClose(shell);
      case 'onTrayIconClick':
        onTrayIconClick?.call(shell);
      case 'onTrayMenuItemClick':
        final key = call.arguments['key'] as String;
        final item = _findMenuItemByKey(key);
        onTrayMenuItemClick?.call(shell, item);
    }
  });
  
  // ... rest of initialization ...
}
```

**Event Descriptions:**

| Event | Trigger | Common Use |
| ----- | ------- | ---------- |
| `onWindowClose` | User clicks window close button | Hide to tray instead of quitting |
| `onTrayIconClick` | User clicks tray icon | Show menu (Windows/macOS) |
| `onTrayMenuItemClick` | User selects menu item | Handle menu actions |

**Usage Example:**

```dart
final result = await initialize(
  trayIcon: 'assets/icon.png',
  trayItems: [
    MenuItem(key: 'show', label: 'Show Window'),
    MenuItem.separator(),
    MenuItem(key: 'quit', label: 'Quit'),
  ],
  onWindowClose: (shell) async {
    // Intercept close - hide to tray instead
    await shell.hideWindow();
  },
  onTrayIconClick: (shell) async {
    // Windows/macOS: show menu on click
    if (!Platform.isLinux) {
      await shell.popUpContextMenu();
    }
  },
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

**Why Callback Pattern:**

- Simple - single function per event
- No listener management (add/remove)
- Set once at initialization
- Functional style matches Result pattern

## Platform-Specific Behavior

  MenuItem.separator(),
  MenuItem(key: 'quit', label: 'Quit'),
]);

menuResult.map((_) => print('Menu set successfully'));

```

## Platform-Specific Behavior

### `popUpContextMenu()` - Menu Display

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
Future<Result<(), TrayError>> popUpContextMenu();

// Linux: returns Ok(()) immediately (no-op)
// Windows: calls TrackPopupMenu
// macOS: calls performClick
```

**User Code Pattern:**

```dart
void onTrayIconClick() async {
  if (!Platform.isLinux) {
    await shell.popUpContextMenu();
  }
  // Linux: menu appears automatically
}
```

### Summary

All platforms expose identical Dart API, but internal implementation differs:

- Linux: System-controlled menu display
- Windows/macOS: Programmatic menu display

### Step 9: Example App

**File:** `example/lib/main.dart`

**Requirements:**

- Initialize shell in main()
- Handle window close by hiding to tray
- Tray menu items: Show Window, Quit
- UI buttons: Show, Hide, Focus, Quit

### Step 10: Build and Test

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
- [ ] Window close button hides to tray
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

## References

- window_manager: `/home/aimer/devel/flutter/window_manager`
- tray_manager: `/home/aimer/devel/flutter/tray_manager`
- GObject docs: <https://docs.gtk.org/gobject/>
- Flutter Linux plugins: <https://docs.flutter.dev/desktop#linux>
