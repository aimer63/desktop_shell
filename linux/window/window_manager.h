#ifndef DESKTOP_SHELL_WINDOW_MANAGER_H_
#define DESKTOP_SHELL_WINDOW_MANAGER_H_

#include <gtk/gtk.h>

typedef struct _WindowManager WindowManager;

// Create a new window manager instance
WindowManager* window_manager_new(GtkWindow* window);

// Destroy window manager and cleanup
void window_manager_destroy(WindowManager* manager);

// Show window
gboolean window_manager_show(WindowManager* manager);

// Hide window
gboolean window_manager_hide(WindowManager* manager);

// Focus window
gboolean window_manager_focus(WindowManager* manager);

// Set prevent close
gboolean window_manager_set_prevent_close(WindowManager* manager,
                                          gboolean prevent);

#endif  // DESKTOP_SHELL_WINDOW_MANAGER_H_
