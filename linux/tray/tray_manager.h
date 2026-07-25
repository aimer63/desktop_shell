#ifndef DESKTOP_SHELL_TRAY_MANAGER_H_
#define DESKTOP_SHELL_TRAY_MANAGER_H_

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DESKTOP_SHELL_TYPE_TRAY_MANAGER desktop_shell_tray_manager_get_type()
G_DECLARE_FINAL_TYPE(DesktopShellTrayManager, desktop_shell_tray_manager,
                     DESKTOP_SHELL, TRAY_MANAGER, GObject)

// Create a new tray manager instance
DesktopShellTrayManager* desktop_shell_tray_manager_new(FlMethodChannel* channel);

// Destroy tray manager and cleanup
void desktop_shell_tray_manager_destroy(DesktopShellTrayManager* manager);

// Set the tray icon
gboolean desktop_shell_tray_manager_set_icon(DesktopShellTrayManager* manager,
                                              const gchar* icon_path);

// Set the tray menu
gboolean desktop_shell_tray_manager_set_menu(DesktopShellTrayManager* manager,
                                              FlValue* menu_items);

G_END_DECLS

#endif  // DESKTOP_SHELL_TRAY_MANAGER_H_
