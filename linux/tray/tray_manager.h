#ifndef DESKTOP_SHELL_TRAY_MANAGER_H_
#define DESKTOP_SHELL_TRAY_MANAGER_H_

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

typedef struct _DesktopShellTrayManager DesktopShellTrayManager;

// Create a new tray manager instance
DesktopShellTrayManager* desktop_shell_tray_manager_new();

// Destroy tray manager and cleanup
void desktop_shell_tray_manager_destroy(DesktopShellTrayManager* manager);

// Initialize the tray manager
gboolean desktop_shell_tray_manager_initialize(DesktopShellTrayManager* manager);

// Set the tray icon
gboolean desktop_shell_tray_manager_set_icon(DesktopShellTrayManager* manager,
                                              const gchar* icon_path);

// Set the tray menu
gboolean desktop_shell_tray_manager_set_menu(DesktopShellTrayManager* manager,
                                              FlValue* menu_items,
                                              FlMethodChannel* channel);

#endif  // DESKTOP_SHELL_TRAY_MANAGER_H_
