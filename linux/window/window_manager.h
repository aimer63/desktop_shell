#ifndef DESKTOP_SHELL_WINDOW_MANAGER_H_
#define DESKTOP_SHELL_WINDOW_MANAGER_H_

#include <gtk/gtk.h>

typedef struct _DesktopShellWindowManager DesktopShellWindowManager;

// Create a new window manager instance
DesktopShellWindowManager* desktop_shell_window_manager_new(GtkWindow* window);

// Destroy window manager and cleanup
void desktop_shell_window_manager_destroy(DesktopShellWindowManager* manager);

// Show window
gboolean desktop_shell_window_manager_show(DesktopShellWindowManager* manager);

// Hide window
gboolean desktop_shell_window_manager_hide(DesktopShellWindowManager* manager);

// Focus window
gboolean desktop_shell_window_manager_focus(DesktopShellWindowManager* manager);

// Set prevent close
gboolean desktop_shell_window_manager_set_prevent_close(
    DesktopShellWindowManager* manager,
    gboolean prevent);

#endif  // DESKTOP_SHELL_WINDOW_MANAGER_H_
