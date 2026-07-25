#ifndef DESKTOP_SHELL_WINDOW_MANAGER_H_
#define DESKTOP_SHELL_WINDOW_MANAGER_H_

#include <windows.h>

#include <functional>

namespace desktop_shell {

class WindowManager {
 public:
  explicit WindowManager(HWND hwnd);
  ~WindowManager();

  // Disallow copy and assign.
  WindowManager(const WindowManager&) = delete;
  WindowManager& operator=(const WindowManager&) = delete;

  // Window operations.
  bool Show();
  bool Hide();
  bool Focus();
  bool SetPreventClose(bool prevent);
  bool Destroy();

  // Getters.
  bool IsPreventClose() const { return prevent_close_; }

  // Event callbacks.
  std::function<void()> on_window_close;

 private:
  HWND hwnd_;
  bool prevent_close_ = false;
};

}  // namespace desktop_shell

#endif  // DESKTOP_SHELL_WINDOW_MANAGER_H_
