# Desktop Shell Design Document

## Overview

Desktop shell combines system tray and window management functionality into a
unified Flutter desktop plugin. This document describes the design decisions
and architecture.

## Design Philosophy

**Goal:** Create a maintainable, testable plugin with proper encapsulation.

**Key principle:** State belongs to instances, not files. While Flutter desktop
is single-instance, using instance-based storage provides:

- Clear ownership of resources
- Easier testing (can create/destroy instances)
- Better separation of concerns
- Alignment with GObject patterns

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

- Manage system tray icon
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
- Manage "prevent close" state

**Key design points:**

- Stores window pointer and signal handler ID
- Delete event handler for intercepting close
- Simple boolean state for prevent_close

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

## CMake Configuration

### apply_standard_settings Function

**Where defined:** Flutter SDK template
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

**Why plugins use it:**
Flutter comment in template (lines 38-41):
> "Be cautious about adding new options here, as plugins use this function by
> default. In most cases, you should add new options to specific targets instead
> of modifying this function."

**Usage in plugin CMakeLists.txt:**

```cmake
apply_standard_settings(${PLUGIN_NAME})
```

**Availability:**

- Function is defined in app's `linux/CMakeLists.txt`
- Plugin CMakeLists.txt is subdirectory'd from app
- Function available in parent scope
- Standard Flutter plugin contract

**When the function is generated:**

The `apply_standard_settings` function is created during `flutter create`:

**Step 1: `flutter create --platforms=linux my_app`**

- Flutter copies template files from SDK
- Template: `flutter_tools/templates/app/linux.tmpl/CMakeLists.txt.tmpl`
- Generates app's `linux/CMakeLists.txt` with function definition

**Step 2: Build process**

```
flutter build linux
    ↓
App CMakeLists.txt processes first
    ↓
Function apply_standard_settings defined
    ↓
add_subdirectory(plugin/linux) called
    ↓
Plugin CMakeLists.txt processes
    ↓
Plugin calls apply_standard_settings()
```

**Important:** This function only exists if the app was created with
`flutter create --platforms=linux`. Manually created CMake files will lack it.

### Library Dependencies

**Required packages:**

- `libappindicator3-dev` or `libayatana-appindicator3-dev`
- GTK3 development files
- pkg-config

**CMake detection:**

```cmake
pkg_check_modules(APPINDICATOR IMPORTED_TARGET ayatana-appindicator3-0.1)
if(NOT APPINDICATOR_FOUND)
  pkg_check_modules(APPINDICATOR IMPORTED_TARGET appindicator3-0.1)
endif()
```

## Method Call Flow

```
Dart -> Platform Channel -> desktop_shell_plugin.cc
                                    |
            +-----------------------+-----------------------+
            |                                               |
    Tray Methods (setTrayIcon, setTrayMenu)  Window Methods (showWindow, hideWindow)
            |                                               |
    tray_manager.cc                                window_manager.cc
            |                                               |
    libappindicator                                  GTK Window APIs
```

## Error Handling Strategy

**Dart API:** All operations return `Result<(), Error>` or `Result<Option<T>,
Error>`.

**Error types:** Specific error per operation with `Option<String> code`:

```dart
final class TrayIconError extends TrayError {
  final String details;
  final Option<String> code;  // Error code (e.g., FILE_NOT_FOUND)
  const TrayIconError(this.details, {this.code = const None()});
}
```

**Native responses:** All methods return Map with `success`, `message`, and
optional `code` fields:

```cpp
// Success
fl_value_set_string(response, "success", fl_value_new_bool(true));

// Error
fl_value_set_string(response, "success", fl_value_new_bool(false));
fl_value_set_string(response, "code", fl_value_new_string("FILE_NOT_FOUND"));
fl_value_set_string(response, "message", fl_value_new_string("Icon not found"));
```

## Platform Channel Protocol

### Method Names

**Tray Methods:**

- `setTrayIcon` - Set tray icon from path
- `setTrayMenu` - Set context menu items
- `popUpContextMenu` - Show context menu

**Window Methods:**

- `showWindow` - Show hidden window
- `hideWindow` - Hide window to tray
- `focusWindow` - Bring window to front
- `setPreventClose` - Intercept close button

**Lifecycle:**

- `destroy` - Cleanup resources

### Events (Native -> Dart)

Events use **callback pattern** passed to `initialize()`:

```dart
Future<Result<DesktopShell, DesktopShellError>> initialize({
  required void Function(DesktopShell) onWindowClose,
  void Function(DesktopShell)? onTrayIconClick,
  void Function(DesktopShell, MenuItem)? onTrayMenuItemClick,
})
```

- `onWindowClose` - User clicks close button (required, use to hide to tray)
- `onTrayIconClick` - User clicks tray icon (Windows/macOS only)
- `onTrayMenuItemClick` - User selects menu item

## Platform-Specific Behavior

### `popUpContextMenu()`

| Platform | Mechanism | User Action |
| -------- | --------- | ----------- |
| **Linux** | Automatic | None - menu appears on click |
| **Windows** | `TrackPopupMenu(hMenu, x, y)` | Must call in `onTrayIconClick` |
| **macOS** | `performClick()` on statusItem | Must call in `onTrayIconClick` |

**Why:** Linux AppIndicator handles clicks automatically. Windows requires
explicit `TrackPopupMenu` call. macOS simulates click via `performClick`.

## CMake Configuration

### Requirements

- `libappindicator3-dev` or `libayatana-appindicator3-dev`
- `pkg-config` for library detection
- GTK3 development files

### Key Settings

```cmake
# Enable deprecated warnings (for appindicator)
# but do not treat as errors
target_compile_options(${PLUGIN_NAME} PRIVATE
  -Wno-deprecated-declarations
)

# Link required libraries
target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  PkgConfig::APPINDICATOR
  ${GTK_LIBRARIES}
)
```

## Future Considerations

### Potential Extensions

1. **Multi-window support** - Architecture supports it
2. **Custom tray icons** - SVG support, animation
3. **Window state preservation** - Remember position/size
4. **Keyboard shortcuts** - Global hotkeys

### Known Limitations

1. **Linux only** - Windows and macOS need separate implementations
2. **Single tray** - One tray icon per app instance
3. **AppIndicator deprecated** - Future: migrate to StatusNotifierItem
4. **No multi-monitor awareness** - Tray appears on primary

## Testing Strategy

### Unit Testing

Difficult for native code due to GTK/AppIndicator dependencies. Consider:

- Mocking GTK functions
- Integration tests via example app
- Manual testing on target platforms

### Integration Testing

Use example app to verify:

1. Tray icon appears
2. Menu items work
3. Window show/hide/focus work
4. Close interception works
5. Cleanup works (no crashes on exit)

## Lessons Learned

1. **Copy working patterns** - tray_manager and window_manager work; use their
   patterns
2. **GObject is complex** - Requires understanding of type system and
   reference counting
3. **Test incrementally** - Build and test each component separately
4. **Documentation matters** - Line numbers and explanations save time
5. **Admit mistakes** - Original plugins were correct; my implementation had
   bugs

## References

- tray_manager: `/home/aimer/devel/flutter/tray_manager`
- window_manager: `/home/aimer/devel/flutter/window_manager`
- GObject documentation: <https://docs.gtk.org/gobject/>
- Flutter Linux plugins: <https://docs.flutter.dev/desktop#linux>
