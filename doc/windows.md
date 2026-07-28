# Windows Implementation Plan

## Overview

Implement Windows support for desktop_shell with modern visual styles and proper menu behavior. Addresses the two main issues from the original tray_manager plugin.

## Issues from Original Plugin

Based on tray-manager-fork.md analysis, the original plugin had two critical Windows issues:

### Issue 1: Menu Theme (Ugly, No Theme Support)

**Problem:** The original tray_manager used raw Win32 `TrackPopupMenu()` without enabling visual styles, resulting in:

- Windows 95/XP classic appearance
- No dark mode support
- No system accent colors
- No HiDPI awareness

**Root Cause:** Visual styles are opt-in on Windows. Without a manifest or activation context, Win32 menus use the classic unthemed look.

**Solution:** Enable Visual Styles via manifest

**Implementation:**

Create `windows/runner/app.manifest`:

```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <dependency>
    <dependentAssembly>
      <assemblyIdentity
          type="win32"
          name="Microsoft.Windows.Common-Controls"
          version="6.0.0.0"
          processorArchitecture="*"
          publicKeyToken="6595b64144ccf1df"
          language="*" />
    </dependentAssembly>
  </dependency>
</assembly>
```

Link manifest in `windows/CMakeLists.txt`:

```cmake
# Enable visual styles for modern menu appearance
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/runner/app.manifest")
```

**Result:**

- Automatic light/dark mode support
- System accent colors
- Windows 11 rounded corners
- HiDPI scaling
- All modern Windows theming features

### Issue 2: Menu Dismissal (Click Outside Doesn't Close)

**Problem:** The original plugin's menu stayed open when clicking outside of it. Users expect the menu to close when clicking elsewhere on the screen.

**Root Cause:** Incorrect `TrackPopupMenu()` flags and missing focus handling.

**Solution:** Proper TPM flags and window message handling

**Implementation:**

In `windows/desktop_shell_plugin.cpp`:

```cpp
// Fix 1: Use correct TrackPopupMenu flags
TrackPopupMenu(
  hMenu,
  TPM_LEFTALIGN | TPM_TOPALIGN |  // Position below tray icon, not above
  TPM_LEFTBUTTON |               // Respond to left clicks
  TPM_RIGHTBUTTON,               // Respond to right clicks
  x, y,
  0,
  hwnd,
  nullptr
);

// Fix 2: Handle WM_KILLFOCUS in window procedure
case WM_KILLFOCUS:
  // Close menu when window loses focus
  if (menuOpen) {
    EndMenu();
    menuOpen = false;
  }
  break;

// Fix 3: Handle WM_ACTIVATEAPP
case WM_ACTIVATEAPP:
  if (!wParam) {  // App being deactivated
    if (menuOpen) {
      EndMenu();
      menuOpen = false;
    }
  }
  break;
```

**Alternative approach:** Use `TPM_NONOTIFY` and handle all messages manually for full control.

## File Structure

```
windows/
├── CMakeLists.txt                    # Build config with manifest linking
├── desktop_shell_plugin.cpp          # Main plugin implementation
├── desktop_shell_plugin.h            # Plugin header
├── tray/
│   ├── tray_icon.cpp                 # Tray icon and menu
│   └── tray_icon.h                   # Tray icon header
├── window/
│   ├── window_manager.cpp            # Window operations
│   └── window_manager.h              # Window manager header
└── runner/
    └── app.manifest                  # Visual styles manifest
```

## Implementation Steps

### Step 1: Create Visual Styles Manifest

Create `windows/runner/app.manifest` with Common Controls v6 dependency.

### Step 2: Update CMakeLists.txt

Link the manifest and configure Windows-specific settings:

```cmake
cmake_minimum_required(VERSION 3.14)
project(desktop_shell LANGUAGES CXX)

set(PLUGIN_NAME "desktop_shell_plugin")

add_library(${PLUGIN_NAME} SHARED
  "desktop_shell_plugin.cpp"
  "tray/tray_icon.cpp"
  "window/window_manager.cpp"
)

# Enable visual styles
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/runner/app.manifest")

# Apply standard settings
apply_standard_settings(${PLUGIN_NAME})

set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE flutter flutter_wrapper_plugin)
```

### Step 3: Implement Tray Icon

Create `windows/tray/tray_icon.cpp`:

```cpp
#include "tray_icon.h"
#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ICON 1001

class TrayIcon {
 private:
  HWND hwnd;
  HMENU hMenu;
  NOTIFYICONDATA nid;
  bool menuOpen;

 public:
  TrayIcon(HWND window) : hwnd(window), menuOpen(false) {
    hMenu = CreatePopupMenu();
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
  }

  void SetIcon(const wchar_t* iconPath) {
    nid.hIcon = (HICON)LoadImage(
      nullptr, iconPath, IMAGE_ICON,
      0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    Shell_NotifyIcon(NIM_ADD, &nid);
  }

  void SetMenu(HMENU menu) {
    if (hMenu) DestroyMenu(hMenu);
    hMenu = menu;
  }

  void ShowMenu(int x, int y) {
    SetForegroundWindow(hwnd);
    menuOpen = true;
    
    TrackPopupMenu(
      hMenu,
      TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
      x, y, 0, hwnd, nullptr);
    
    menuOpen = false;
  }

  void HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
      case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
          POINT pt;
          GetCursorPos(&pt);
          ShowMenu(pt.x, pt.y);
        }
        break;
        
      case WM_KILLFOCUS:
      case WM_ACTIVATEAPP:
        if (menuOpen) {
          EndMenu();
          menuOpen = false;
        }
        break;
    }
  }
};
```

### Step 4: Implement Window Manager

Create `windows/window/window_manager.cpp`:

```cpp
#include "window_manager.h"
#include <windows.h>

class WindowManager {
 private:
  HWND hwnd;
  bool preventClose;

 public:
  WindowManager(HWND window) : hwnd(window), preventClose(false) {}

  void Show() {
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
  }

  void Hide() {
    ShowWindow(hwnd, SW_HIDE);
  }

  void Focus() {
    SetForegroundWindow(hwnd);
  }

  void SetPreventClose(bool prevent) {
    preventClose = prevent;
  }

  bool HandleClose() {
    if (preventClose) {
      Hide();
      return true;  // Prevent close
    }
    return false;  // Allow close
  }
};
```

### Step 5: Implement Main Plugin

Create `windows/desktop_shell_plugin.cpp`:

```cpp
#include "desktop_shell_plugin.h"
#include "tray/tray_icon.h"
#include "window/window_manager.h"
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

namespace desktop_shell {

class DesktopShellPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  DesktopShellPlugin(flutter::PluginRegistrarWindows* registrar);
  virtual ~DesktopShellPlugin();

 private:
  flutter::PluginRegistrarWindows* registrar_;
  std::unique_ptr<TrayIcon> tray_icon_;
  std::unique_ptr<WindowManager> window_manager_;
  HWND hwnd_;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
      
  std::optional<LRESULT> HandleWindowProc(HWND hwnd, UINT message,
                                          WPARAM wparam, LPARAM lparam);
};

void DesktopShellPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto plugin = std::make_unique<DesktopShellPlugin>(registrar);
  registrar->AddPlugin(std::move(plugin));
}

DesktopShellPlugin::DesktopShellPlugin(flutter::PluginRegistrarWindows* registrar)
    : registrar_(registrar) {
  hwnd_ = registrar->GetView()->GetNativeWindow();
  tray_icon_ = std::make_unique<TrayIcon>(hwnd_);
  window_manager_ = std::make_unique<WindowManager>(hwnd_);
  
  registrar->RegisterTopLevelWindowProcDelegate(
      [this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        return this->HandleWindowProc(hwnd, message, wparam, lparam);
      });
}

DesktopShellPlugin::~DesktopShellPlugin() {}

std::optional<LRESULT> DesktopShellPlugin::HandleWindowProc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  tray_icon_->HandleMessage(message, wparam, lparam);
  
  switch (message) {
    case WM_CLOSE:
      if (window_manager_->HandleClose()) {
        return 0;  // Prevent default close
      }
      break;
  }
  
  return std::nullopt;
}

void DesktopShellPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto& method = method_call.method_name();
  
  if (method == "setTrayIcon") {
    // Implementation
    result->Success();
  } else if (method == "setTrayMenu") {
    // Implementation
    result->Success();
  } else if (method == "show") {
    window_manager_->Show();
    result->Success();
  } else if (method == "hide") {
    window_manager_->Hide();
    result->Success();
  } else if (method == "focus") {
    window_manager_->Focus();
    result->Success();
  } else if (method == "setPreventClose") {
    const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (args) {
      auto it = args->find(flutter::EncodableValue("prevent"));
      if (it != args->end()) {
        bool prevent = std::get<bool>(it->second);
        window_manager_->SetPreventClose(prevent);
      }
    }
    result->Success();
  } else {
    result->NotImplemented();
  }
}

}  // namespace desktop_shell
```

## Testing Checklist

### Visual Styles

- [ ] Menu appears with modern Windows 10/11 styling
- [ ] Menu respects system light/dark theme
- [ ] Menu uses system accent color
- [ ] Menu renders correctly on HiDPI displays

### Menu Dismissal

- [ ] Menu closes when clicking outside
- [ ] Menu closes when clicking on another window
- [ ] Menu closes when pressing Escape
- [ ] Menu closes when app loses focus

### Functionality

- [ ] Tray icon appears on startup
- [ ] Tray icon responds to left/right click
- [ ] Menu items are clickable
- [ ] "Show" brings window to front
- [ ] "Quit" exits application
- [ ] Close button minimizes to tray (when setPreventClose enabled)

## Notes

1. **Visual Styles:** The manifest approach is the standard Windows way to enable modern theming. It requires no code changes beyond adding the manifest file and linker flag.

2. **Menu Dismissal:** The combination of `SetForegroundWindow()` before `TrackPopupMenu()` and handling `WM_KILLFOCUS`/`WM_ACTIVATEAPP` ensures proper menu behavior.

3. **Single Instance:** Consider using a mutex for single-instance enforcement on Windows.

4. **Icon Format:** Windows uses `.ico` files. Ensure the example app includes a proper icon resource.

## References

- [Visual Styles Overview](https://docs.microsoft.com/en-us/windows/win32/controls/visual-styles-overview)
- [TrackPopupMenu Documentation](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-trackpopupmenu)
- [Shell_NotifyIcon Documentation](https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw)
- Original tray_manager Windows implementation issues (see tray-manager-fork.md)
