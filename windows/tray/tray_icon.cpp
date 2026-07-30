#include "tray_icon.h"

#include <windows.h>
#include <flutter/standard_method_codec.h>

#include <string>

namespace desktop_shell {

// Helper function to convert UTF-8 string to wide string using Windows API
static std::wstring Utf8ToWide(const std::string& utf8) {
  if (utf8.empty()) {
    return std::wstring();
  }
  int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
  if (wide_len <= 0) {
    return std::wstring();
  }
  std::wstring wide(wide_len - 1, 0);  // -1 to exclude null terminator
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], wide_len);
  return wide;
}

TrayIcon::TrayIcon(flutter::MethodChannel<flutter::EncodableValue>* channel)
    : channel_(channel), hwnd_(nullptr), icon_(nullptr), menu_(nullptr) {}

TrayIcon::~TrayIcon() {
  Destroy();
}

bool TrayIcon::Initialize(HWND hwnd) {
  hwnd_ = hwnd;
  return true;
}

void TrayIcon::SetWindowHandle(HWND hwnd) {
  hwnd_ = hwnd;
}

bool TrayIcon::SetIcon(const std::string& icon_path) {
  
  if (!hwnd_) {
    return false;
  }

  
  // Convert icon path to wide string
  std::wstring wide_path = Utf8ToWide(icon_path);
  

  // Load the icon
  HICON hIcon = static_cast<HICON>(
      LoadImageW(nullptr, wide_path.c_str(), IMAGE_ICON, 0, 0,
                 LR_LOADFROMFILE | LR_DEFAULTSIZE));

  if (!hIcon) {
    // Try loading as a small icon
    hIcon = static_cast<HICON>(
        LoadImageW(nullptr, wide_path.c_str(), IMAGE_ICON, 16, 16,
                   LR_LOADFROMFILE));
  }

  if (!hIcon) {
    return false;
  }
  

  // Destroy old icon if exists
  if (icon_) {
    DestroyIcon(icon_);
  }

  icon_ = hIcon;

  // Add or update the tray icon
  NOTIFYICONDATAW nid = {};
  nid.cbSize = sizeof(NOTIFYICONDATAW);
  nid.hWnd = hwnd_;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_TRAYMESSAGE;
  nid.hIcon = icon_;

  // Set tooltip (app name)
  wcscpy_s(nid.szTip, L"desktop_shell");

  if (!icon_added_) {
    if (Shell_NotifyIconW(NIM_ADD, &nid)) {
      icon_added_ = true;
    } else {
    }
  } else {
    Shell_NotifyIconW(NIM_MODIFY, &nid);
  }

  return icon_added_;
}

bool TrayIcon::SetMenu(const flutter::EncodableList& menu_items) {
  
  // Store menu items for later use
  menu_items_ = menu_items;
  
  return true;
}

bool TrayIcon::PopUpContextMenu() {
  
  if (!hwnd_ || menu_items_.empty()) {
    return false;
  }

  // Create popup menu
  HMENU hMenu = CreatePopupMenu();
  if (!hMenu) {
    return false;
  }
  

  // Build menu from items
  for (const auto& item : menu_items_) {
    if (std::holds_alternative<flutter::EncodableMap>(item)) {
      auto map = std::get<flutter::EncodableMap>(item);

      // Get ID from menu item (hashCode from Dart)
      int id = 0;
      auto id_it = map.find(flutter::EncodableValue("id"));
      if (id_it != map.end() &&
          std::holds_alternative<int>(id_it->second)) {
        id = std::get<int>(id_it->second);
      }

      std::string label;
      std::string type = "normal";

      auto label_it = map.find(flutter::EncodableValue("label"));
      if (label_it != map.end() &&
          std::holds_alternative<std::string>(label_it->second)) {
        label = std::get<std::string>(label_it->second);
      }

      auto type_it = map.find(flutter::EncodableValue("type"));
      if (type_it != map.end() &&
          std::holds_alternative<std::string>(type_it->second)) {
        type = std::get<std::string>(type_it->second);
      }

      if (type == "separator") {
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
      } else {
        std::wstring wide_label = Utf8ToWide(label);
        AppendMenuW(hMenu, MF_STRING, id, wide_label.c_str());
      }
    }
  }

  // Get cursor position
  POINT pt;
  GetCursorPos(&pt);

  // Show menu
  SetForegroundWindow(hwnd_);
  TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
                 pt.x, pt.y, 0, hwnd_, nullptr);

  DestroyMenu(hMenu);

  return true;
}

bool TrayIcon::Destroy() {
  
  if (icon_added_) {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd_;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    icon_added_ = false;
  }

  if (icon_) {
    DestroyIcon(icon_);
    icon_ = nullptr;
  }

  return true;
}

}  // namespace desktop_shell
