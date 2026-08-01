#include "tray_menu.h"
#include "tray_icon.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

// Forward declaration for callback
static void on_menu_item_activate(GtkMenuItem* item, gpointer user_data);

GtkWidget* tray_menu_build(TrayIcon* tray, FlValue* items) {
  GtkWidget* menu = gtk_menu_new();

  if (items == NULL || fl_value_get_type(items) != FL_VALUE_TYPE_LIST) {
    gtk_widget_show(menu);
    return menu;
  }

  for (gint i = 0; i < fl_value_get_length(items); i++) {
    FlValue* item = fl_value_get_list_value(items, i);
    if (item == NULL || fl_value_get_type(item) != FL_VALUE_TYPE_MAP) {
      continue;
    }

    FlValue* label_value = fl_value_lookup_string(item, "label");
    FlValue* type_value = fl_value_lookup_string(item, "type");
    FlValue* id_value = fl_value_lookup_string(item, "id");
    FlValue* disabled_value = fl_value_lookup_string(item, "disabled");

    const gchar* type = "normal";
    if (type_value != NULL &&
        fl_value_get_type(type_value) == FL_VALUE_TYPE_STRING) {
      type = fl_value_get_string(type_value);
    }

    if (strcmp(type, "separator") == 0) {
      GtkWidget* separator = gtk_separator_menu_item_new();
      gtk_widget_show(separator);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);
    } else {
      const gchar* label = "";
      if (label_value != NULL &&
          fl_value_get_type(label_value) == FL_VALUE_TYPE_STRING) {
        label = fl_value_get_string(label_value);
      }

      gint item_id = 0;
      if (id_value != NULL &&
          fl_value_get_type(id_value) == FL_VALUE_TYPE_INT) {
        item_id = fl_value_get_int(id_value);
      }

      GtkWidget* menu_item = gtk_menu_item_new_with_label(label);

      if (disabled_value != NULL &&
          fl_value_get_type(disabled_value) == FL_VALUE_TYPE_BOOL) {
        if (fl_value_get_bool(disabled_value)) {
          gtk_widget_set_sensitive(menu_item, FALSE);
        }
      }

      g_object_set_data(G_OBJECT(menu_item), "tray", tray);
      g_signal_connect(G_OBJECT(menu_item), "activate",
                       G_CALLBACK(on_menu_item_activate),
                       GINT_TO_POINTER(item_id));

      gtk_widget_show(menu_item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }
  }

  gtk_widget_show(menu);
  return menu;
}

static void on_menu_item_activate(GtkMenuItem* item, gpointer user_data) {
  (void)item;
  gint id = GPOINTER_TO_INT(user_data);
  TrayIcon* tray =
      TRAY_ICON(g_object_get_data(G_OBJECT(item), "tray"));

  if (tray != NULL) {
    FlMethodChannel* channel = tray_icon_get_channel(tray);
    if (channel != NULL) {
      g_autoptr(FlValue) args = fl_value_new_map();
      fl_value_set_string_take(args, "id", fl_value_new_int(id));
      fl_method_channel_invoke_method(channel, "onTrayMenuItemClick", args,
                                      NULL, NULL, NULL);
    }
  }
}
