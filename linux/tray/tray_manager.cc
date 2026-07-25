#include "tray_manager.h"

#include <gio/gio.h>

struct _DesktopShellTrayManager {
  GObject parent_instance;
  GDBusConnection* connection;
  gchar* icon_name;
  GMenu* menu;
  guint watcher_id;
  guint owner_id;
  FlMethodChannel* channel;
};

static void desktop_shell_tray_manager_init(DesktopShellTrayManager* self) {
  self->connection = nullptr;
  self->icon_name = nullptr;
  self->menu = nullptr;
  self->watcher_id = 0;
  self->owner_id = 0;
  self->channel = nullptr;
}

static void desktop_shell_tray_manager_finalize(GObject* object) {
  DesktopShellTrayManager* self = (DesktopShellTrayManager*)object;

  if (self->watcher_id > 0) {
    g_bus_unwatch_name(self->watcher_id);
  }

  if (self->owner_id > 0) {
    g_bus_unown_name(self->owner_id);
  }

  if (self->connection != nullptr) {
    g_object_unref(self->connection);
  }

  g_free(self->icon_name);

  if (self->menu != nullptr) {
    g_object_unref(self->menu);
  }

  G_OBJECT_CLASS(g_type_class_peek_parent(G_OBJECT_GET_CLASS(object)))->finalize(object);
}

static void desktop_shell_tray_manager_class_init(GObjectClass* klass) {
  klass->finalize = desktop_shell_tray_manager_finalize;
}

G_DEFINE_TYPE(DesktopShellTrayManager, desktop_shell_tray_manager, G_TYPE_OBJECT)

DesktopShellTrayManager* desktop_shell_tray_manager_new() {
  return (DesktopShellTrayManager*)g_object_new(desktop_shell_tray_manager_get_type(), nullptr);
}

void desktop_shell_tray_manager_destroy(DesktopShellTrayManager* manager) {
  if (manager != nullptr) {
    g_object_unref(manager);
  }
}

// Menu item activated callback
static void menu_item_activated(GSimpleAction* action,
                                GVariant* parameter,
                                gpointer user_data) {
  const gchar* key = g_action_get_name(G_ACTION(action));
  FlMethodChannel* channel = (FlMethodChannel*)user_data;

  if (channel != nullptr) {
    g_autoptr(FlValue) args = fl_value_new_map();
    fl_value_set_string_take(args, "id", fl_value_new_int(g_str_hash(key)));
    fl_method_channel_invoke_method(channel, "onTrayMenuItemClick", args, nullptr, nullptr, nullptr);
  }
}

// Build GTK menu from FlValue list
static void build_menu_from_value(GMenu* menu, FlValue* items, GActionGroup* action_group) {
  if (items == nullptr || fl_value_get_type(items) != FL_VALUE_TYPE_LIST) {
    return;
  }

  size_t length = fl_value_get_length(items);
  for (size_t i = 0; i < length; i++) {
    FlValue* item = fl_value_get_list_value(items, i);
    if (item == nullptr || fl_value_get_type(item) != FL_VALUE_TYPE_MAP) {
      continue;
    }

    FlValue* label_value = fl_value_lookup_string(item, "label");
    FlValue* type_value = fl_value_lookup_string(item, "type");
    FlValue* key_value = fl_value_lookup_string(item, "key");

    const gchar* type = "normal";
    if (type_value != nullptr && fl_value_get_type(type_value) == FL_VALUE_TYPE_STRING) {
      type = fl_value_get_string(type_value);
    }

    if (strcmp(type, "separator") == 0) {
      g_menu_append(menu, "─", nullptr);
    } else {
      const gchar* label = "";
      if (label_value != nullptr && fl_value_get_type(label_value) == FL_VALUE_TYPE_STRING) {
        label = fl_value_get_string(label_value);
      }

      const gchar* key = "";
      if (key_value != nullptr && fl_value_get_type(key_value) == FL_VALUE_TYPE_STRING) {
        key = fl_value_get_string(key_value);
      }

      // Create action for this menu item
      GSimpleAction* action = g_simple_action_new(key, nullptr);
      g_signal_connect(action, "activate", G_CALLBACK(menu_item_activated), nullptr);
      g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));

      g_menu_append(menu, label, key);
    }
  }
}

gboolean desktop_shell_tray_manager_initialize(DesktopShellTrayManager* manager) {
  if (manager == nullptr) {
    return FALSE;
  }

  // For now, use a simple GTK StatusIcon-based approach
  // In the future, this should use StatusNotifierItem for better compatibility
  manager->menu = g_menu_new();

  return TRUE;
}

gboolean desktop_shell_tray_manager_set_icon(DesktopShellTrayManager* manager,
                                              const gchar* icon_path) {
  if (manager == nullptr) {
    return FALSE;
  }

  g_free(manager->icon_name);
  manager->icon_name = g_strdup(icon_path);

  // TODO: Implement StatusNotifierItem icon registration
  // For now, just store the icon name

  return TRUE;
}

gboolean desktop_shell_tray_manager_set_menu(DesktopShellTrayManager* manager,
                                              FlValue* menu_items,
                                              FlMethodChannel* channel) {
  if (manager == nullptr) {
    return FALSE;
  }

  manager->channel = channel;

  // Clear existing menu
  g_menu_remove_all(manager->menu);

  // Create action group
  GSimpleActionGroup* action_group = g_simple_action_group_new();

  // Build menu from items
  build_menu_from_value(manager->menu, menu_items, G_ACTION_GROUP(action_group));

  return TRUE;
}
