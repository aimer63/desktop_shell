#ifndef DESKTOP_SHELL_TRAY_ICON_H_
#define DESKTOP_SHELL_TRAY_ICON_H_

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TRAY_ICON_TYPE tray_icon_get_type()
G_DECLARE_FINAL_TYPE(TrayIcon, tray_icon, TRAY, ICON, GObject)

// Create a new tray icon instance
TrayIcon* tray_icon_new(FlMethodChannel* channel);

// Destroy tray icon and cleanup
void tray_icon_destroy(TrayIcon* self);

// Set the tray icon
gboolean tray_icon_set_icon(TrayIcon* self, const gchar* icon_path);

// Set the tray menu
gboolean tray_icon_set_menu(TrayIcon* self, FlValue* menu_items);

// Get the channel (for internal use by tray_menu)
FlMethodChannel* tray_icon_get_channel(TrayIcon* self);

G_END_DECLS

#endif  // DESKTOP_SHELL_TRAY_ICON_H_
