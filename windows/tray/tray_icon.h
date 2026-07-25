#ifndef DESKTOP_SHELL_TRAY_ICON_H_
#define DESKTOP_SHELL_TRAY_ICON_H_

#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

#include <string>
#include <vector>

namespace desktop_shell {

class TrayIcon {
 public:
  explicit TrayIcon(flutter::MethodChannel<flutter::EncodableValue>* channel);
  ~TrayIcon();

  // Disallow copy and assign.
  TrayIcon(const TrayIcon&) = delete;
  TrayIcon& operator=(const TrayIcon&) = delete;

  // Initialize with parent window handle.
  bool Initialize(HWND hwnd);

  // Set the tray icon from file path.
  bool SetIcon(const std::string& icon_path);

  // Set the context menu items.
  bool SetMenu(const flutter::EncodableList& menu_items);

  // Show the context menu at cursor position.
  bool PopUpContextMenu();

  // Remove the tray icon and cleanup.
  bool Destroy();

 private:
  // Window procedure for tray messages.
  static LRESULT CALLBACK TrayWindowProc(HWND hwnd,
                                         UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam);

  flutter::MethodChannel<flutter::EncodableValue>* channel_;
  HWND hwnd_;
  HICON icon_;
  HMENU menu_;
  bool icon_added_ = false;
  flutter::EncodableList menu_items_;
};

}  // namespace desktop_shell

#endif  // DESKTOP_SHELL_TRAY_ICON_H_
