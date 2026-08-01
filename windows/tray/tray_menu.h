#ifndef DESKTOP_SHELL_TRAY_MENU_H_
#define DESKTOP_SHELL_TRAY_MENU_H_

#include <windows.h>

#include <flutter/standard_method_codec.h>

#include <string>
#include <vector>

namespace desktop_shell {

// Build and show the tray context menu
bool ShowTrayContextMenu(
    HWND hwnd,
    const std::vector<flutter::EncodableValue>& menu_items);

}  // namespace desktop_shell

#endif  // DESKTOP_SHELL_TRAY_MENU_H_
