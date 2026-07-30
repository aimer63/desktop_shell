#include "desktop_shell/desktop_shell_plugin.h"

#include <windows.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "tray/tray_icon.h"
#include "window/window_manager.h"

// Dark mode support - undocumented Windows APIs
// These are internal APIs used by Microsoft Explorer and system apps
namespace DarkMode {
enum PreferredAppMode { Default, AllowDark, ForceDark, ForceLight, Max };
using fnSetPreferredAppMode = PreferredAppMode (WINAPI *)(PreferredAppMode);
using fnFlushMenuThemes = void (WINAPI *)();

// Store function pointer for runtime theme changes
static fnFlushMenuThemes g_FlushMenuThemes = nullptr;

static void Init() {
  HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr,
      LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (hUxtheme) {
    auto SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    g_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));

    if (SetPreferredAppMode && g_FlushMenuThemes) {
      SetPreferredAppMode(AllowDark);  // Allow dark, follows system theme
      g_FlushMenuThemes();
    }
  }
}

static void Refresh() {
  if (g_FlushMenuThemes) {
    g_FlushMenuThemes();  // Re-flush when theme changes at runtime
  }
}
}

namespace desktop_shell {

// Forward declarations
class TrayIcon;
class WindowManager;

// C++ Plugin class (internal implementation)
class DesktopShellPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  DesktopShellPlugin(
      flutter::PluginRegistrarWindows* registrar,
      std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel);

  virtual ~DesktopShellPlugin();

  // Disallow copy and assign.
  DesktopShellPlugin(const DesktopShellPlugin&) = delete;
  DesktopShellPlugin& operator=(const DesktopShellPlugin&) = delete;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // The registrar for this plugin, for accessing the window.
  flutter::PluginRegistrarWindows* registrar_;

  // The method channel used to communicate with the Dart side.
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;

  // Tray icon and menu manager.
  std::unique_ptr<TrayIcon> tray_icon_;

  // Window manager.
  std::unique_ptr<WindowManager> window_manager_;

  // Window message procedure ID.
  int window_proc_id_ = -1;

  // Handle window messages.
  std::optional<LRESULT> HandleWindowMessage(HWND hwnd,
                                               UINT message,
                                               WPARAM wparam,
                                               LPARAM lparam);
};

// static
void DesktopShellPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  
  // Initialize dark mode support for menus
  DarkMode::Init();
  
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "desktop_shell",
          &flutter::StandardMethodCodec::GetInstance());

  auto* channel_ptr = channel.get();

  auto plugin = std::make_unique<DesktopShellPlugin>(registrar,
                                                       std::move(channel));

  // Initialize tray icon with channel (window handle set later)
  plugin->tray_icon_ = std::make_unique<TrayIcon>(channel_ptr);
  plugin->tray_icon_->Initialize(nullptr);

  // Initialize window manager (window handle set later)
  plugin->window_manager_ = std::make_unique<WindowManager>();

  // Register window message handler
  plugin->window_proc_id_ = registrar->RegisterTopLevelWindowProcDelegate(
      [plugin_ptr = plugin.get()](HWND hwnd, UINT message, WPARAM wparam,
                                   LPARAM lparam) {
        return plugin_ptr->HandleWindowMessage(hwnd, message, wparam, lparam);
      });

  registrar->AddPlugin(std::move(plugin));
}

DesktopShellPlugin::DesktopShellPlugin(
    flutter::PluginRegistrarWindows* registrar,
    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel)
    : registrar_(registrar), channel_(std::move(channel)) {
  channel_->SetMethodCallHandler([this](const auto& call, auto result) {
    HandleMethodCall(call, std::move(result));
  });
}

DesktopShellPlugin::~DesktopShellPlugin() {
  if (window_proc_id_ != -1) {
    registrar_->UnregisterTopLevelWindowProcDelegate(window_proc_id_);
  }
}

void DesktopShellPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const std::string& method = method_call.method_name();

  // Helper to create success response Map
  auto create_success_response = [&result](const std::string& message) {
    flutter::EncodableMap response;
    response[flutter::EncodableValue("success")] =
        flutter::EncodableValue(true);
    response[flutter::EncodableValue("message")] =
        flutter::EncodableValue(message);
    result->Success(flutter::EncodableValue(response));
  };

  // Helper to create error response Map with optional code
  auto create_error_response = [&result](const std::string& message,
                                         const std::string& code = "") {
    flutter::EncodableMap response;
    response[flutter::EncodableValue("success")] =
        flutter::EncodableValue(false);
    response[flutter::EncodableValue("message")] =
        flutter::EncodableValue(message);
    if (!code.empty()) {
      response[flutter::EncodableValue("code")] =
          flutter::EncodableValue(code);
    }
    result->Success(flutter::EncodableValue(response));
  };

  try {
    if (method == "initialize") {
      
      // Get the native window handle (now available since Dart is running)
      HWND hwnd = ::GetAncestor(registrar_->GetView()->GetNativeWindow(),
                                GA_ROOT);
      

      if (!hwnd) {
        create_error_response("Failed to get window handle", "GET_HWND_FAILED");
        return;
      }

      // Set window handle on tray icon and window manager
      if (tray_icon_) {
        tray_icon_->SetWindowHandle(hwnd);
      }
      if (window_manager_) {
        window_manager_->SetWindowHandle(hwnd);
      }

      create_success_response("OK");

    } else if (method == "setTrayIcon") {
      
      if (!tray_icon_) {
        create_error_response("Tray not initialized", "TRAY_NOT_INITIALIZED");
        return;
      }

      const auto* arguments =
          std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (!arguments) {
        create_error_response("Invalid arguments", "INVALID_ARGS");
        return;
      }

      auto it = arguments->find(flutter::EncodableValue("iconPath"));
      if (it == arguments->end() ||
          !std::holds_alternative<std::string>(it->second)) {
        create_error_response("Missing iconPath", "MISSING_ICONPATH");
        return;
      }

      std::string icon_path = std::get<std::string>(it->second);
      if (tray_icon_->SetIcon(icon_path)) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to set tray icon", "SET_ICON_FAILED");
      }

    } else if (method == "setTrayMenu") {
      if (!tray_icon_) {
        create_error_response("Tray not initialized", "TRAY_NOT_INITIALIZED");
        return;
      }

      const auto* arguments =
          std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (!arguments) {
        create_error_response("Invalid arguments", "INVALID_ARGS");
        return;
      }

      auto it = arguments->find(flutter::EncodableValue("menu"));
      if (it == arguments->end() ||
          !std::holds_alternative<flutter::EncodableList>(it->second)) {
        create_error_response("Missing or invalid menu", "MISSING_MENU");
        return;
      }

      const auto& menu = std::get<flutter::EncodableList>(it->second);
      if (tray_icon_->SetMenu(menu)) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to set tray menu", "SET_MENU_FAILED");
      }

    } else if (method == "popUpTrayMenu") {
      if (!tray_icon_) {
        create_error_response("Tray not initialized", "TRAY_NOT_INITIALIZED");
        return;
      }

      if (tray_icon_->PopUpContextMenu()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to show context menu", "POPUP_FAILED");
      }

    } else if (method == "show") {
      if (!window_manager_) {
        create_error_response("Window manager not initialized", "WINDOW_MANAGER_NOT_INITIALIZED");
        return;
      }

      if (window_manager_->Show()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to show window", "SHOW_FAILED");
      }

    } else if (method == "hide") {
      if (!window_manager_) {
        create_error_response("Window manager not initialized", "WINDOW_MANAGER_NOT_INITIALIZED");
        return;
      }

      if (window_manager_->Hide()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to hide window", "HIDE_FAILED");
      }

    } else if (method == "focus") {
      if (!window_manager_) {
        create_error_response("Window manager not initialized", "WINDOW_MANAGER_NOT_INITIALIZED");
        return;
      }

      if (window_manager_->Focus()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to focus window", "FOCUS_FAILED");
      }

    } else if (method == "setPreventClose") {
      
      if (!window_manager_) {
        create_error_response("Window manager not initialized", "WINDOW_MANAGER_NOT_INITIALIZED");
        return;
      }

      const auto* arguments =
          std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (!arguments) {
        create_error_response("Invalid arguments", "INVALID_ARGS");
        return;
      }

      auto it = arguments->find(flutter::EncodableValue("prevent"));
      if (it == arguments->end() ||
          !std::holds_alternative<bool>(it->second)) {
        create_error_response("Missing or invalid prevent flag", "MISSING_PREVENT_FLAG");
        return;
      }

      bool prevent = std::get<bool>(it->second);
      if (window_manager_->SetPreventClose(prevent)) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to set prevent close", "SET_PREVENT_CLOSE_FAILED");
      }

    } else if (method == "destroy") {
      if (tray_icon_) {
        tray_icon_->Destroy();
      }
      if (window_manager_) {
        window_manager_->Destroy();
      }
      create_success_response("OK");

    } else {
      result->NotImplemented();
    }
  } catch (const std::exception& e) {
    create_error_response(std::string("Exception: ") + e.what(), "EXCEPTION");
  } catch (...) {
    create_error_response("Unknown exception", "EXCEPTION");
  }
}

std::optional<LRESULT> DesktopShellPlugin::HandleWindowMessage(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam) {

  // Handle theme changes at runtime
  if (message == WM_SETTINGCHANGE && lparam) {
    // lParam points to string indicating what changed
    // "ImmersiveColorSet" = light/dark mode toggle
    if (wcscmp(reinterpret_cast<LPCWSTR>(lparam),
               L"ImmersiveColorSet") == 0) {
      DarkMode::Refresh();  // Refresh menu theming
    }
  }

  // Handle window close interception
  if (message == WM_CLOSE && window_manager_ &&
      window_manager_->IsPreventClose()) {
    // Hide instead of close
    window_manager_->Hide();

    // Notify Flutter
    if (channel_) {
      channel_->InvokeMethod("onWindowClose", nullptr);
    }

    return 0;  // Prevent default close
  }

  // Handle tray icon messages
  if (message == WM_TRAYMESSAGE && tray_icon_) {
    switch (lparam) {
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
        // Notify Flutter
        if (channel_) {
          channel_->InvokeMethod("onTrayIconClick", nullptr);
        }
        return 0;
      default:
        break;
    }
  }

  // Handle menu item clicks
  if (message == WM_COMMAND && tray_icon_) {
    // Use full wparam value (sequential IDs from Dart are 1024-65535)
    int menu_id = static_cast<int>(wparam);
    if (menu_id >= 1024 && channel_) {
      flutter::EncodableMap args;
      args[flutter::EncodableValue("id")] =
          flutter::EncodableValue(menu_id);
      channel_->InvokeMethod("onTrayMenuItemClick",
                           std::make_unique<flutter::EncodableValue>(args));
      return 0;
    }
  }

  return std::nullopt;  // Let default handling continue
}

}  // namespace desktop_shell

// C export function for Flutter plugin registration
void DesktopShellPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  desktop_shell::DesktopShellPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
