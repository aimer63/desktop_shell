#ifndef DESKTOP_SHELL_TRAY_MENU_H_
#define DESKTOP_SHELL_TRAY_MENU_H_

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include "tray_icon.h"

G_BEGIN_DECLS

// Build a menu for the tray icon
GtkWidget* tray_menu_build(TrayIcon* tray, FlValue* items);

G_END_DECLS

#endif  // DESKTOP_SHELL_TRAY_MENU_H_
