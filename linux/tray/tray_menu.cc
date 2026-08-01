#include "tray_menu.h"
#include "tray_icon.h"

#include <flutter_linux/flutter_linux.h>

// Forward declaration for callback
static void on_menu_item_activate(GSimpleAction* action,
                                  GVariant* parameter,
                                  gpointer user_data);

void tray_menu_build(TrayIcon* tray, FlValue* items) {
  g_print("[MENU] tray_menu_build called\n");
  
  // Create new menu model
  GMenu* menu = g_menu_new();
  g_print("[MENU] Created GMenu at %p\n", (void*)menu);

  // Create new action group
  GSimpleActionGroup* actions = g_simple_action_group_new();
  g_print("[MENU] Created GSimpleActionGroup at %p\n", (void*)actions);

  if (items == nullptr || fl_value_get_type(items) != FL_VALUE_TYPE_LIST) {
    g_print("[MENU] WARNING: items is null or not a list, creating empty menu\n");
    tray_icon_set_menu_model(tray, menu);
    tray_icon_set_actions_group(tray, actions);
    g_object_unref(menu);
    g_object_unref(actions);
    return;
  }

  gint item_count = fl_value_get_length(items);
  g_print("[MENU] Processing %d menu items\n", item_count);

  gint items_added = 0;
  for (gint i = 0; i < fl_value_get_length(items); i++) {
    FlValue* item = fl_value_get_list_value(items, i);
    g_print("[MENU] Processing item %d\n", i);
    
    if (item == nullptr || fl_value_get_type(item) != FL_VALUE_TYPE_MAP) {
      g_print("[MENU]   Skipping: item is null or not a map\n");
      continue;
    }

    FlValue* label_value = fl_value_lookup_string(item, "label");
    FlValue* type_value = fl_value_lookup_string(item, "type");
    FlValue* id_value = fl_value_lookup_string(item, "id");
    FlValue* disabled_value = fl_value_lookup_string(item, "disabled");

    const gchar* type = "normal";
    if (type_value != nullptr &&
        fl_value_get_type(type_value) == FL_VALUE_TYPE_STRING) {
      type = fl_value_get_string(type_value);
    }

    g_print("[MENU]   Type: %s\n", type);

    if (strcmp(type, "separator") == 0) {
      g_print("[MENU]   Skipping separator (not implemented)\n");
      continue;
    } else {
      const gchar* label = "";
      if (label_value != nullptr &&
          fl_value_get_type(label_value) == FL_VALUE_TYPE_STRING) {
        label = fl_value_get_string(label_value);
      }

      gint item_id = 0;
      if (id_value != nullptr &&
          fl_value_get_type(id_value) == FL_VALUE_TYPE_INT) {
        item_id = fl_value_get_int(id_value);
      }

      // Create unique action name
      gchar* action_name = g_strdup_printf("item_%d", item_id);
      GSimpleAction* action = g_simple_action_new(action_name, nullptr);

      // Store mapping data on the action
      g_object_set_data(G_OBJECT(action), "item_id", GINT_TO_POINTER(item_id));
      g_object_set_data(G_OBJECT(action), "tray", tray);

      // Connect activate signal
      g_signal_connect(G_OBJECT(action), "activate",
                       G_CALLBACK(on_menu_item_activate), nullptr);

      // Add to action group
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
      g_object_unref(action);

      // Add menu item with full action path
      gchar* full_action = g_strdup_printf("indicator.%s", action_name);
      g_print("[MENU]   Adding menu item: label='%s', action='%s'\n", label, full_action);
      g_menu_append(menu, label, full_action);
      g_print("[MENU]   Menu item appended successfully\n");

      // Handle disabled state
      if (disabled_value != nullptr &&
          fl_value_get_type(disabled_value) == FL_VALUE_TYPE_BOOL) {
        if (fl_value_get_bool(disabled_value)) {
          GAction* gaction = g_action_map_lookup_action(
              G_ACTION_MAP(actions), action_name);
          if (gaction != nullptr) {
            g_simple_action_set_enabled(G_SIMPLE_ACTION(gaction), FALSE);
          }
        }
      }

      g_free(action_name);
      g_free(full_action);
      items_added++;
    }
  }

  g_print("[MENU] Total items added: %d\n", items_added);
  g_print("[MENU] Setting menu model and actions group on tray\n");
  tray_icon_set_menu_model(tray, menu);
  tray_icon_set_actions_group(tray, actions);
  g_print("[MENU] Unreffing local menu and actions\n");
  g_object_unref(menu);
  g_object_unref(actions);
  g_print("[MENU] tray_menu_build complete\n");
}

static void on_menu_item_activate(GSimpleAction* action,
                                  GVariant* parameter,
                                  gpointer user_data) {
  (void)parameter;
  (void)user_data;

  g_print("[MENU] on_menu_item_activate CALLED!\n");

  gint id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "item_id"));
  TrayIcon* tray = TRAY_ICON(g_object_get_data(G_OBJECT(action), "tray"));

  g_print("[MENU]   item_id: %d, tray: %p\n", id, (void*)tray);

  if (tray != nullptr) {
    FlMethodChannel* channel = tray_icon_get_channel(tray);
    g_print("[MENU]   channel: %p\n", (void*)channel);
    if (channel != nullptr) {
      g_print("[MENU]   Invoking Dart callback onTrayMenuItemClick\n");
      g_autoptr(FlValue) args = fl_value_new_map();
      fl_value_set_string_take(args, "id", fl_value_new_int(id));
      fl_method_channel_invoke_method(channel, "onTrayMenuItemClick", args,
                                      nullptr, nullptr, nullptr);
    } else {
      g_print("[MENU]   ERROR: channel is null!\n");
    }
  } else {
    g_print("[MENU]   ERROR: tray is null!\n");
  }
}
