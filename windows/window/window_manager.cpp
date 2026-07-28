#include "window_manager.h"

#include <windows.h>
#include <flutter/standard_method_codec.h>

namespace desktop_shell {

WindowManager::WindowManager() : hwnd_(nullptr) {
  OutputDebugStringA("DEBUG_WINDOW: WindowManager constructor\n");
}

WindowManager::~WindowManager() {
  OutputDebugStringA("DEBUG_WINDOW: WindowManager destructor\n");
}

void WindowManager::SetWindowHandle(HWND hwnd) {
  OutputDebugStringA("DEBUG_WINDOW: SetWindowHandle called\n");
  hwnd_ = hwnd;
}

bool WindowManager::Show() {
  OutputDebugStringA("DEBUG_WINDOW: Show() started\n");
  if (!hwnd_) {
    OutputDebugStringA("DEBUG_WINDOW: Show() hwnd_ is null!\n");
    return false;
  }
  BOOL result = ShowWindow(hwnd_, SW_SHOW);
  OutputDebugStringA("DEBUG_WINDOW: Show() returning\n");
  return result != 0;
}

bool WindowManager::Hide() {
  OutputDebugStringA("DEBUG_WINDOW: Hide() started\n");
  if (!hwnd_) {
    OutputDebugStringA("DEBUG_WINDOW: Hide() hwnd_ is null!\n");
    return false;
  }
  BOOL result = ShowWindow(hwnd_, SW_HIDE);
  OutputDebugStringA("DEBUG_WINDOW: Hide() returning\n");
  return result != 0;
}

bool WindowManager::Focus() {
  OutputDebugStringA("DEBUG_WINDOW: Focus() started\n");
  if (!hwnd_) {
    OutputDebugStringA("DEBUG_WINDOW: Focus() hwnd_ is null!\n");
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

  OutputDebugStringA("DEBUG_WINDOW: Focus() returning\n");
  return true;
}

bool WindowManager::SetPreventClose(bool prevent) {
  OutputDebugStringA("DEBUG_WINDOW: SetPreventClose() started\n");
  prevent_close_ = prevent;
  OutputDebugStringA("DEBUG_WINDOW: SetPreventClose() returning\n");
  return true;
}

bool WindowManager::Destroy() {
  OutputDebugStringA("DEBUG_WINDOW: Destroy() started\n");
  if (!hwnd_) {
    OutputDebugStringA("DEBUG_WINDOW: Destroy() hwnd_ is null!\n");
    return false;
  }
  BOOL result = DestroyWindow(hwnd_);
  OutputDebugStringA("DEBUG_WINDOW: Destroy() returning\n");
  return result != 0;
}

}  // namespace desktop_shell
