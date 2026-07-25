#include "tray_manager.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#ifdef HAVE_AYATANA
#include <libayatana-appindicator/app-indicator.h>
#else
#include <libappindicator/app-indicator.h>
#endif

struct _DesktopShellTrayManager {
  GObject parent_instance;
  FlMethodChannel* channel;
  AppIndicator* indicator;
  GtkWidget* menu;
};

G_DEFINE_TYPE(DesktopShellTrayManager, desktop_shell_tray_manager, G_TYPE_OBJECT)

static void desktop_shell_tray_manager_dispose(GObject* object) {
  DesktopShellTrayManager* self = DESKTOP_SHELL_TRAY_MANAGER(object);

  if (self->indicator != nullptr) {
    app_indicator_set_status(self->indicator, APP_INDICATOR_STATUS_PASSIVE);
    g_clear_object(&self->indicator);
  }

  if (self->menu != nullptr) {
    gtk_widget_destroy(self->menu);
    self->menu = nullptr;
  }

  g_clear_object(&self->channel);

  G_OBJECT_CLASS(desktop_shell_tray_manager_parent_class)->dispose(object);
}

static void desktop_shell_tray_manager_class_init(DesktopShellTrayManagerClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = desktop_shell_tray_manager_dispose;
}

static void desktop_shell_tray_manager_init(DesktopShellTrayManager* self) {
  self->channel = nullptr;
  self->indicator = nullptr;
  self->menu = nullptr;
}

DesktopShellTrayManager* desktop_shell_tray_manager_new(FlMethodChannel* channel) {
  DesktopShellTrayManager* manager = DESKTOP_SHELL_TRAY_MANAGER(
      g_object_new(desktop_shell_tray_manager_get_type(), nullptr));
  manager->channel = FL_METHOD_CHANNEL(g_object_ref(channel));
  return manager;
}

void desktop_shell_tray_manager_destroy(DesktopShellTrayManager* manager) {
  if (manager != nullptr) {
    g_object_unref(manager);
  }
}

static void on_menu_item_activate(GtkMenuItem* item, gpointer user_data) {
  gint id = GPOINTER_TO_INT(user_data);
  DesktopShellTrayManager* self = DESKTOP_SHELL_TRAY_MANAGER(g_object_get_data(G_OBJECT(item), "manager"));

  if (self->channel != nullptr) {
    g_autoptr(FlValue) args = fl_value_new_map();
    fl_value_set_string_take(args, "id", fl_value_new_int(id));
    fl_method_channel_invoke_method(self->channel, "onTrayMenuItemClick", args,
                                    nullptr, nullptr, nullptr);
  }
}

static GtkWidget* build_menu(DesktopShellTrayManager* self, FlValue* items) {
  GtkWidget* menu = gtk_menu_new();

  if (items == nullptr || fl_value_get_type(items) != FL_VALUE_TYPE_LIST) {
    return menu;
  }

  for (gint i = 0; i < fl_value_get_length(items); i++) {
    FlValue* item = fl_value_get_list_value(items, i);
    if (item == nullptr || fl_value_get_type(item) != FL_VALUE_TYPE_MAP) {
      continue;
    }

    FlValue* label_value = fl_value_lookup_string(item, "label");
    FlValue* type_value = fl_value_lookup_string(item, "type");
    FlValue* id_value = fl_value_lookup_string(item, "id");
    FlValue* disabled_value = fl_value_lookup_string(item, "disabled");

    const gchar* type = "normal";
    if (type_value != nullptr && fl_value_get_type(type_value) == FL_VALUE_TYPE_STRING) {
      type = fl_value_get_string(type_value);
    }

    if (strcmp(type, "separator") == 0) {
      GtkWidget* separator = gtk_separator_menu_item_new();
      gtk_widget_show(separator);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);
    } else {
      const gchar* label = "";
      if (label_value != nullptr && fl_value_get_type(label_value) == FL_VALUE_TYPE_STRING) {
        label = fl_value_get_string(label_value);
      }

      gint item_id = 0;
      if (id_value != nullptr && fl_value_get_type(id_value) == FL_VALUE_TYPE_INT) {
        item_id = fl_value_get_int(id_value);
      }

      GtkWidget* menu_item = gtk_menu_item_new_with_label(label);

      if (disabled_value != nullptr && fl_value_get_type(disabled_value) == FL_VALUE_TYPE_BOOL) {
        if (fl_value_get_bool(disabled_value)) {
          gtk_widget_set_sensitive(menu_item, FALSE);
        }
      }

      g_object_set_data(G_OBJECT(menu_item), "manager", self);
      g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(on_menu_item_activate),
                       GINT_TO_POINTER(item_id));

      gtk_widget_show(menu_item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }
  }

  gtk_widget_show(menu);
  return menu;
}

gboolean desktop_shell_tray_manager_set_icon(DesktopShellTrayManager* manager,
                                              const gchar* icon_path) {
  if (manager == nullptr) {
    return FALSE;
  }

  // Create indicator if not exists
  if (manager->indicator == nullptr) {
    manager->indicator = app_indicator_new("desktop_shell", icon_path,
                                           APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    
    // Create empty menu initially
    if (manager->menu == nullptr) {
      manager->menu = gtk_menu_new();
      gtk_widget_show(manager->menu);
    }
    
    app_indicator_set_menu(manager->indicator, GTK_MENU(manager->menu));
  }

  app_indicator_set_status(manager->indicator, APP_INDICATOR_STATUS_ACTIVE);
  app_indicator_set_icon_full(manager->indicator, icon_path, "");

  return TRUE;
}

gboolean desktop_shell_tray_manager_set_menu(DesktopShellTrayManager* manager,
                                              FlValue* menu_items) {
  if (manager == nullptr) {
    return FALSE;
  }

  // Destroy old menu
  if (manager->menu != nullptr) {
    gtk_widget_destroy(manager->menu);
    manager->menu = nullptr;
  }

  // Build new menu
  manager->menu = build_menu(manager, menu_items);

  if (manager->indicator != nullptr) {
    app_indicator_set_menu(manager->indicator, GTK_MENU(manager->menu));
  }

  return TRUE;
}
