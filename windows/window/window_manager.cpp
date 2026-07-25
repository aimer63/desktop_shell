#include "window_manager.h"

#include <flutter/standard_method_codec.h>

namespace desktop_shell {

WindowManager::WindowManager(HWND hwnd) : hwnd_(hwnd) {}

WindowManager::~WindowManager() = default;

bool WindowManager::Show() {
  if (!hwnd_) {
    return false;
  }
  return ShowWindow(hwnd_, SW_SHOW) != 0;
}

bool WindowManager::Hide() {
  if (!hwnd_) {
    return false;
  }
  return ShowWindow(hwnd_, SW_HIDE) != 0;
}

bool WindowManager::Focus() {
  if (!hwnd_) {
    return false;
  }

  // Bring window to front and activate it
  HWND foreground_window = GetForegroundWindow();
  DWORD foreground_thread = GetWindowThreadProcessId(foreground_window, nullptr);
  DWORD current_thread = GetCurrentThreadId();

  if (foreground_thread != current_thread) {
    AttachThreadInput(foreground_thread, current_thread, TRUE);
  }

  SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
  SetWindowPos(hwnd_, HWND_NOTOPMOST, 0, 0, 0, 0,
               SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
  SetForegroundWindow(hwnd_);
  SetFocus(hwnd_);

  if (foreground_thread != current_thread) {
    AttachThreadInput(foreground_thread, current_thread, FALSE);
  }

  return true;
}

bool WindowManager::SetPreventClose(bool prevent) {
  prevent_close_ = prevent;
  return true;
}

bool WindowManager::Destroy() {
  if (!hwnd_) {
    return false;
  }
  return DestroyWindow(hwnd_) != 0;
}

}  // namespace desktop_shell
