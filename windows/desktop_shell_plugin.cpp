#include "desktop_shell/desktop_shell_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "tray/tray_icon.h"
#include "window/window_manager.h"

namespace desktop_shell {

// static
void DesktopShellPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "desktop_shell",
          &flutter::StandardMethodCodec::GetInstance());

  auto* channel_ptr = channel.get();

  auto plugin = std::make_unique<DesktopShellPlugin>(registrar,
                                                      std::move(channel));

  // Initialize tray icon with channel
  plugin->tray_icon_ = std::make_unique<TrayIcon>(channel_ptr);

  // Get the native window handle
  HWND hwnd = nullptr;
  if (registrar->GetView()) {
    hwnd = registrar->GetView()->GetNativeWindow();
  }

  // Initialize tray and window managers
  if (hwnd) {
    plugin->tray_icon_->Initialize(hwnd);
    plugin->window_manager_ = std::make_unique<WindowManager>(hwnd);
  }

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

  // Helper to wrap success
  auto success = [&result]() {
    result->Success(flutter::EncodableValue(true));
  };

  // Helper to wrap error
  auto error = [&result](const std::string& message) {
    flutter::EncodableMap error_map;
    error_map[flutter::EncodableValue("error")] =
        flutter::EncodableValue(true);
    error_map[flutter::EncodableValue("message")] =
        flutter::EncodableValue(message);
    result->Success(flutter::EncodableValue(error_map));
  };

  try {
    if (method == "setTrayIcon") {
      if (!tray_icon_) {
        error("Tray not initialized");
        return;
      }

      const auto* arguments =
          std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (!arguments) {
        error("Invalid arguments");
        return;
      }

      auto it = arguments->find(flutter::EncodableValue("iconPath"));
      if (it == arguments->end() ||
          !std::holds_alternative<std::string>(it->second)) {
        error("Missing iconPath");
        return;
      }

      std::string icon_path = std::get<std::string>(it->second);
      if (tray_icon_->SetIcon(icon_path)) {
        success();
      } else {
        error("Failed to set tray icon");
      }

    } else if (method == "setTrayMenu") {
      if (!tray_icon_) {
        error("Tray not initialized");
        return;
      }

      const auto* arguments =
          std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (!arguments) {
        error("Invalid arguments");
        return;
      }

      auto it = arguments->find(flutter::EncodableValue("menu"));
      if (it == arguments->end() ||
          !std::holds_alternative<flutter::EncodableList>(it->second)) {
        error("Missing menu");
        return;
      }

      const auto& menu = std::get<flutter::EncodableList>(it->second);
      if (tray_icon_->SetMenu(menu)) {
        success();
      } else {
        error("Failed to set tray menu");
      }

    } else if (method == "popUpTrayMenu") {
      if (!tray_icon_) {
        error("Tray not initialized");
        return;
      }

      if (tray_icon_->PopUpContextMenu()) {
        success();
      } else {
        error("Failed to show context menu");
      }

    } else if (method == "show") {
      if (!window_manager_) {
        error("Window manager not initialized");
        return;
      }

      if (window_manager_->Show()) {
        success();
      } else {
        error("Failed to show window");
      }

    } else if (method == "hide") {
      if (!window_manager_) {
        error("Window manager not initialized");
        return;
      }

      if (window_manager_->Hide()) {
        success();
      } else {
        error("Failed to hide window");
      }

    } else if (method == "focus") {
      if (!window_manager_) {
        error("Window manager not initialized");
        return;
      }

      if (window_manager_->Focus()) {
        success();
      } else {
        error("Failed to focus window");
      }

    } else if (method == "setPreventClose") {
      if (!window_manager_) {
        error("Window manager not initialized");
        return;
      }

      const auto* arguments =
          std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (!arguments) {
        error("Invalid arguments");
        return;
      }

      auto it = arguments->find(flutter::EncodableValue("prevent"));
      if (it == arguments->end() ||
          !std::holds_alternative<bool>(it->second)) {
        error("Missing prevent flag");
        return;
      }

      bool prevent = std::get<bool>(it->second);
      if (window_manager_->SetPreventClose(prevent)) {
        success();
      } else {
        error("Failed to set prevent close");
      }

    } else if (method == "destroy") {
      if (tray_icon_) {
        tray_icon_->Destroy();
      }
      if (window_manager_) {
        window_manager_->Destroy();
      }
      success();

    } else {
      result->NotImplemented();
    }
  } catch (const std::exception& e) {
    error(std::string("Exception: ") + e.what());
  } catch (...) {
    error("Unknown exception");
  }
}

std::optional<LRESULT> DesktopShellPlugin::HandleWindowMessage(
    HWND hwnd,
    UINT message,
    WPARAM wparam,
    LPARAM lparam) {
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
