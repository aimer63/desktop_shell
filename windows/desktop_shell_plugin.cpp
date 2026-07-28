#include "desktop_shell/desktop_shell_plugin.h"

#include <windows.h>
#include <stdio.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "tray/tray_icon.h"
#include "window/window_manager.h"

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
  OutputDebugStringA("DEBUG_PLUGIN: RegisterWithRegistrar started\n");
  
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
  OutputDebugStringA("DEBUG_PLUGIN: RegisterWithRegistrar complete\n");
}

DesktopShellPlugin::DesktopShellPlugin(
    flutter::PluginRegistrarWindows* registrar,
    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel)
    : registrar_(registrar), channel_(std::move(channel)) {
  OutputDebugStringA("DEBUG_PLUGIN: DesktopShellPlugin constructor\n");
  channel_->SetMethodCallHandler([this](const auto& call, auto result) {
    HandleMethodCall(call, std::move(result));
  });
}

DesktopShellPlugin::~DesktopShellPlugin() {
  OutputDebugStringA("DEBUG_PLUGIN: DesktopShellPlugin destructor\n");
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
      OutputDebugStringA("DEBUG_NATIVE: initialize handler started\n");
      
      // Get the native window handle (now available since Dart is running)
      HWND hwnd = ::GetAncestor(registrar_->GetView()->GetNativeWindow(),
                                GA_ROOT);
      
      OutputDebugStringA("DEBUG_NATIVE: Got HWND\n");

      if (!hwnd) {
        OutputDebugStringA("DEBUG_NATIVE: HWND is null!\n");
        create_error_response("Failed to get window handle", "GET_HWND_FAILED");
        return;
      }

      // Set window handle on tray icon and window manager
      if (tray_icon_) {
        tray_icon_->SetWindowHandle(hwnd);
        OutputDebugStringA("DEBUG_NATIVE: SetWindowHandle on tray_icon_\n");
      }
      if (window_manager_) {
        window_manager_->SetWindowHandle(hwnd);
        OutputDebugStringA("DEBUG_NATIVE: SetWindowHandle on window_manager_\n");
      }

      OutputDebugStringA("DEBUG_NATIVE: initialize handler complete\n");
      create_success_response("Initialized successfully");

    } else if (method == "setTrayIcon") {
      OutputDebugStringA("DEBUG_NATIVE: setTrayIcon handler started\n");
      
      if (!tray_icon_) {
        OutputDebugStringA("DEBUG_NATIVE: tray_icon_ is null!\n");
        create_error_response("Tray not initialized", "NOT_INITIALIZED");
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
        create_error_response("Missing iconPath", "INVALID_ARGS");
        return;
      }

      std::string icon_path = std::get<std::string>(it->second);
      if (tray_icon_->SetIcon(icon_path)) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to set tray icon", "SET_FAILED");
      }

    } else if (method == "setTrayMenu") {
      if (!tray_icon_) {
        create_error_response("Tray not initialized", "NOT_INITIALIZED");
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
        create_error_response("Missing menu", "INVALID_ARGS");
        return;
      }

      const auto& menu = std::get<flutter::EncodableList>(it->second);
      if (tray_icon_->SetMenu(menu)) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to set tray menu", "SET_FAILED");
      }

    } else if (method == "popUpTrayMenu") {
      if (!tray_icon_) {
        create_error_response("Tray not initialized", "NOT_INITIALIZED");
        return;
      }

      if (tray_icon_->PopUpContextMenu()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to show context menu", "POPUP_FAILED");
      }

    } else if (method == "show") {
      if (!window_manager_) {
        create_error_response("Window manager not initialized", "NOT_INITIALIZED");
        return;
      }

      if (window_manager_->Show()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to show window", "SHOW_FAILED");
      }

    } else if (method == "hide") {
      if (!window_manager_) {
        create_error_response("Window manager not initialized", "NOT_INITIALIZED");
        return;
      }

      if (window_manager_->Hide()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to hide window", "HIDE_FAILED");
      }

    } else if (method == "focus") {
      if (!window_manager_) {
        create_error_response("Window manager not initialized", "NOT_INITIALIZED");
        return;
      }

      if (window_manager_->Focus()) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to focus window", "FOCUS_FAILED");
      }

    } else if (method == "setPreventClose") {
      OutputDebugStringA("DEBUG_NATIVE: setPreventClose handler started\n");
      
      if (!window_manager_) {
        OutputDebugStringA("DEBUG_NATIVE: window_manager_ is null!\n");
        create_error_response("Window manager not initialized", "NOT_INITIALIZED");
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
        create_error_response("Missing prevent flag", "INVALID_ARGS");
        return;
      }

      bool prevent = std::get<bool>(it->second);
      if (window_manager_->SetPreventClose(prevent)) {
        create_success_response("OK");
      } else {
        create_error_response("Failed to set prevent close", "SET_FAILED");
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
  char msg_buf[256];
  snprintf(msg_buf, sizeof(msg_buf), "DEBUG_PLUGIN_PROC: Message %u, wparam=%llu, lparam=%llu\n", message, (unsigned long long)wparam, (unsigned long long)lparam);
  OutputDebugStringA(msg_buf);
  
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
