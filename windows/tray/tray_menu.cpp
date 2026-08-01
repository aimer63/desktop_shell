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

bool ShowTrayContextMenu(
    HWND hwnd,
    const std::vector<flutter::EncodableValue>& menu_items) {
  if (!hwnd || menu_items.empty()) {
    return false;
  }

  // Create popup menu
  HMENU hMenu = CreatePopupMenu();
  if (!hMenu) {
    return false;
  }

  // Build menu from items
  for (const auto& item : menu_items) {
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
  SetForegroundWindow(hwnd);
  TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
                 pt.x, pt.y, 0, hwnd, nullptr);

  DestroyMenu(hMenu);

  return true;
}

}  // namespace desktop_shell
