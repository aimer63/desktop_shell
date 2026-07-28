#include "tray_icon.h"

#include <windows.h>
#include <stdio.h>
#include <flutter/standard_method_codec.h>

#include <string>

namespace desktop_shell {

// Message ID for tray icon events
static const UINT kTrayIconMessage = WM_USER + 1;

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

  // Register the tray icon message
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = TrayWindowProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = L"DesktopShellTrayWindow";

  if (!RegisterClassExW(&wc)) {
    return false;
  }

  // Store this pointer for window procedure
  if (hwnd_) {
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  }

  return true;
}

void TrayIcon::SetWindowHandle(HWND hwnd) {
  hwnd_ = hwnd;
  if (hwnd_) {
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  }
}

bool TrayIcon::SetIcon(const std::string& icon_path) {
  OutputDebugStringA("DEBUG_TRAY: SetIcon started\n");
  
  if (!hwnd_) {
    OutputDebugStringA("DEBUG_TRAY: hwnd_ is null!\n");
    return false;
  }

  OutputDebugStringA("DEBUG_TRAY: hwnd_ is valid\n");
  
  // Convert icon path to wide string
  std::wstring wide_path = Utf8ToWide(icon_path);
  
  OutputDebugStringA("DEBUG_TRAY: Converted path to wide string\n");

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
    OutputDebugStringA("DEBUG_TRAY: LoadImageW failed for both sizes\n");
    return false;
  }
  
  OutputDebugStringA("DEBUG_TRAY: Icon loaded successfully\n");

  // Destroy old icon if exists
  if (icon_) {
    OutputDebugStringA("DEBUG_TRAY: Destroying old icon\n");
    DestroyIcon(icon_);
  }

  icon_ = hIcon;
  OutputDebugStringA("DEBUG_TRAY: New icon stored\n");

  // Add or update the tray icon
  NOTIFYICONDATAW nid = {};
  nid.cbSize = sizeof(NOTIFYICONDATAW);
  nid.hWnd = hwnd_;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = kTrayIconMessage;
  nid.hIcon = icon_;

  // Set tooltip (app name)
  wcscpy_s(nid.szTip, L"desktop_shell");

  if (!icon_added_) {
    OutputDebugStringA("DEBUG_TRAY: Calling Shell_NotifyIconW(NIM_ADD)\n");
    if (Shell_NotifyIconW(NIM_ADD, &nid)) {
      icon_added_ = true;
      OutputDebugStringA("DEBUG_TRAY: Shell_NotifyIconW(NIM_ADD) succeeded\n");
    } else {
      OutputDebugStringA("DEBUG_TRAY: Shell_NotifyIconW(NIM_ADD) FAILED\n");
    }
  } else {
    OutputDebugStringA("DEBUG_TRAY: Calling Shell_NotifyIconW(NIM_MODIFY)\n");
    Shell_NotifyIconW(NIM_MODIFY, &nid);
  }

  OutputDebugStringA("DEBUG_TRAY: SetIcon returning\n");
  return icon_added_;
}

bool TrayIcon::SetMenu(const flutter::EncodableList& menu_items) {
  OutputDebugStringA("DEBUG_TRAY: SetMenu started\n");
  
  // Store menu items for later use
  menu_items_ = menu_items;
  
  OutputDebugStringA("DEBUG_TRAY: SetMenu returning\n");
  return true;
}

bool TrayIcon::PopUpContextMenu() {
  OutputDebugStringA("DEBUG_TRAY: PopUpContextMenu started\n");
  
  if (!hwnd_ || menu_items_.empty()) {
    OutputDebugStringA("DEBUG_TRAY: PopUpContextMenu early return - hwnd_ or menu_items_ invalid\n");
    return false;
  }

  // Create popup menu
  HMENU hMenu = CreatePopupMenu();
  if (!hMenu) {
    OutputDebugStringA("DEBUG_TRAY: CreatePopupMenu failed\n");
    return false;
  }
  
  OutputDebugStringA("DEBUG_TRAY: Popup menu created\n");

  // Build menu from items
  int id = 1;
  for (const auto& item : menu_items_) {
    if (std::holds_alternative<flutter::EncodableMap>(item)) {
      auto map = std::get<flutter::EncodableMap>(item);

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
        AppendMenuW(hMenu, MF_STRING, id++, wide_label.c_str());
      }
    }
  }

  // Get cursor position
  POINT pt;
  GetCursorPos(&pt);

  // Show menu
  OutputDebugStringA("DEBUG_TRAY: Showing popup menu\n");
  SetForegroundWindow(hwnd_);
  TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
                 pt.x, pt.y, 0, hwnd_, nullptr);

  DestroyMenu(hMenu);
  OutputDebugStringA("DEBUG_TRAY: PopUpContextMenu returning\n");

  return true;
}

bool TrayIcon::Destroy() {
  OutputDebugStringA("DEBUG_TRAY: Destroy started\n");
  
  if (icon_added_) {
    OutputDebugStringA("DEBUG_TRAY: Removing tray icon\n");
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd_;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    icon_added_ = false;
  }

  if (icon_) {
    OutputDebugStringA("DEBUG_TRAY: Destroying icon\n");
    DestroyIcon(icon_);
    icon_ = nullptr;
  }

  OutputDebugStringA("DEBUG_TRAY: Destroy returning\n");
  return true;
}

LRESULT CALLBACK TrayIcon::TrayWindowProc(HWND hwnd,
                                           UINT message,
                                           WPARAM wparam,
                                           LPARAM lparam) {
  char msg_buf[256];
  snprintf(msg_buf, sizeof(msg_buf), "DEBUG_TRAY_PROC: Message %u, wparam=%llu, lparam=%llu\n", message, (unsigned long long)wparam, (unsigned long long)lparam);
  OutputDebugStringA(msg_buf);
  
  TrayIcon* tray = reinterpret_cast<TrayIcon*>(
      GetWindowLongPtr(hwnd, GWLP_USERDATA));

  if (!tray) {
    OutputDebugStringA("DEBUG_TRAY_PROC: No tray object, calling DefWindowProc\n");
    return DefWindowProc(hwnd, message, wparam, lparam);
  }

  if (message == kTrayIconMessage) {
    OutputDebugStringA("DEBUG_TRAY_PROC: Got kTrayIconMessage\n");
    switch (lparam) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
        // Notify Flutter that tray icon was clicked
        if (tray->channel_) {
          tray->channel_->InvokeMethod("onTrayIconClick", nullptr);
        }
        break;

      case WM_COMMAND: {
        // Menu item clicked
        int menu_id = LOWORD(wparam);
        if (tray->channel_ && menu_id > 0) {
          flutter::EncodableMap args;
          args[flutter::EncodableValue("id")] =
              flutter::EncodableValue(menu_id);
          tray->channel_->InvokeMethod("onTrayMenuItemClick",
                                       std::make_unique<flutter::EncodableValue>(args));
        }
        break;
      }
    }
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}

}  // namespace desktop_shell
