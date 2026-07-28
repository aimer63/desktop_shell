#ifndef DESKTOP_SHELL_PLUGIN_H_
#define DESKTOP_SHELL_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace desktop_shell {

class TrayIcon;
class WindowManager;

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

}  // namespace desktop_shell

#endif  // DESKTOP_SHELL_PLUGIN_H_
