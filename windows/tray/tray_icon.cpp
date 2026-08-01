#include "tray_icon.h"
#include "tray_menu.h"

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
  return ShowTrayContextMenu(hwnd_, menu_items_);
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
