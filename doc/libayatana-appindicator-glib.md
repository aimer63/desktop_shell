# Migration Plan: GTK → GLib AppIndicator

Migrate from `libayatana-appindicator` (GTK-based, deprecated) to
`libayatana-appindicator-glib` (GLib-only, v2.0.3).

## Overview

The new GLib-based implementation eliminates GTK dependencies from the tray
functionality, using `GMenu` and `GAction` instead of `GtkMenu` with direct
callbacks.

## Files to Modify

### 1. linux/CMakeLists.txt

**Change:**

```cmake
# OLD:
pkg_check_modules(APPINDICATOR IMPORTED_TARGET ayatana-appindicator3-0.1)
if(APPINDICATOR_FOUND)
  set(HAVE_AYATANA 1)
else()
  pkg_check_modules(APPINDICATOR IMPORTED_TARGET appindicator3-0.1)
endif()

# NEW:
pkg_check_modules(APPINDICATOR REQUIRED IMPORTED_TARGET ayatana-appindicator-glib)
```

**Remove:**

- `HAVE_AYATANA` conditional compile definitions
- Legacy `appindicator3-0.1` fallback

### 2. linux/tray/tray_manager.h

**Change struct fields:**

```cpp
// OLD:
GtkWidget* menu;

// NEW:
GMenu* menu;
GSimpleActionGroup* actions;
```

### 3. linux/tray/tray_manager.cc

#### Header Include

```cpp
// OLD (conditional):
#ifdef HAVE_AYATANA
#include <libayatana-appindicator/app-indicator.h>
#else
#include <libappindicator/app-indicator.h>
#endif

// NEW:
#include <libayatana-appindicator-glib/ayatana-appindicator.h>
```

#### Menu Building (MAJOR CHANGE)

**OLD - GtkMenu with signal callbacks:**

```cpp
static GtkWidget* build_menu(DesktopShellTrayManager* self, FlValue* items) {
  GtkWidget* menu = gtk_menu_new();

  for (gint i = 0; i < fl_value_get_length(items); i++) {
    // ... extract item data ...

    GtkWidget* menu_item = gtk_menu_item_new_with_label(label);
    g_object_set_data(G_OBJECT(menu_item), "manager", self);
    g_signal_connect(G_OBJECT(menu_item), "activate",
                     G_CALLBACK(on_menu_item_activate),
                     GINT_TO_POINTER(item_id));
    gtk_widget_show(menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  }

  gtk_widget_show(menu);
  return menu;
}
```

**NEW - GMenu + GSimpleActionGroup:**

```cpp
static void build_menu_and_actions(DesktopShellTrayManager* self,
                                    FlValue* items) {
  // Create new menu model
  if (self->menu != nullptr) {
    g_object_unref(self->menu);
  }
  self->menu = g_menu_new();

  // Create new action group
  if (self->actions != nullptr) {
    g_object_unref(self->actions);
  }
  self->actions = g_simple_action_group_new();

  if (items == nullptr || fl_value_get_type(items) != FL_VALUE_TYPE_LIST) {
    app_indicator_set_menu(self->indicator, self->menu);
    app_indicator_set_actions(self->indicator, self->actions);
    return;
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
    if (type_value != nullptr &&
        fl_value_get_type(type_value) == FL_VALUE_TYPE_STRING) {
      type = fl_value_get_string(type_value);
    }

    if (strcmp(type, "separator") == 0) {
      g_menu_append(self->menu, "", nullptr);  // Empty label = separator
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

      // Store mapping data
      g_object_set_data(G_OBJECT(action), "item_id", GINT_TO_POINTER(item_id));
      g_object_set_data(G_OBJECT(action), "manager", self);

      // Connect activate signal
      g_signal_connect(G_OBJECT(action), "activate",
                       G_CALLBACK(on_menu_item_activate), nullptr);

      // Add to action group
      g_action_map_add_action(G_ACTION_MAP(self->actions), G_ACTION(action));
      g_object_unref(action);

      // Add menu item with full action path
      gchar* full_action = g_strdup_printf("indicator.%s", action_name);
      g_menu_append(self->menu, label, full_action);

      // Handle disabled state
      if (disabled_value != nullptr &&
          fl_value_get_type(disabled_value) == FL_VALUE_TYPE_BOOL) {
        if (fl_value_get_bool(disabled_value)) {
          GAction* gaction = g_action_map_lookup_action(
              G_ACTION_MAP(self->actions), action_name);
          if (gaction != nullptr) {
            g_simple_action_set_enabled(G_SIMPLE_ACTION(gaction), FALSE);
          }
        }
      }

      g_free(action_name);
      g_free(full_action);
    }
  }

  app_indicator_set_menu(self->indicator, self->menu);
  app_indicator_set_actions(self->indicator, self->actions);
}
```

#### Callback Function

**OLD:**

```cpp
static void on_menu_item_activate(GtkMenuItem* item, gpointer user_data) {
  gint id = GPOINTER_TO_INT(user_data);
  DesktopShellTrayManager* self = DESKTOP_SHELL_TRAY_MANAGER(
      g_object_get_data(G_OBJECT(item), "manager"));

  if (self->channel != nullptr) {
    g_autoptr(FlValue) args = fl_value_new_map();
    fl_value_set_string_take(args, "id", fl_value_new_int(id));
    fl_method_channel_invoke_method(self->channel, "onTrayMenuItemClick",
                                    args, nullptr, nullptr, nullptr);
  }
}
```

**NEW:**

```cpp
static void on_menu_item_activate(GSimpleAction* action,
                                  GVariant* parameter,
                                  gpointer user_data) {
  (void)parameter;  // Unused
  (void)user_data;  // Unused - data stored in action

  gint id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "item_id"));
  DesktopShellTrayManager* self = DESKTOP_SHELL_TRAY_MANAGER(
      g_object_get_data(G_OBJECT(action), "manager"));

  if (self != nullptr && self->channel != nullptr) {
    g_autoptr(FlValue) args = fl_value_new_map();
    fl_value_set_string_take(args, "id", fl_value_new_int(id));
    fl_method_channel_invoke_method(self->channel, "onTrayMenuItemClick",
                                    args, nullptr, nullptr, nullptr);
  }
}
```

#### Cleanup

**OLD:**

```cpp
if (self->menu != nullptr && GTK_IS_WIDGET(self->menu)) {
  gtk_widget_destroy(self->menu);
  self->menu = nullptr;
}
```

**NEW:**

```cpp
if (self->menu != nullptr) {
  g_object_unref(self->menu);
  self->menu = nullptr;
}
if (self->actions != nullptr) {
  g_object_unref(self->actions);
  self->actions = nullptr;
}
```

#### Initialization

**OLD:**

```cpp
// Create empty menu initially
if (manager->menu == nullptr) {
  manager->menu = gtk_menu_new();
  gtk_widget_show(manager->menu);
}
app_indicator_set_menu(manager->indicator, GTK_MENU(manager->menu));
```

**NEW:**

```cpp
// Menu and actions created lazily in set_menu
// No initialization needed here
```

## User Dependency Changes

| Aspect | Before | After |
|--------|--------|-------|
| **Runtime dependency** | `libayatana-appindicator` | `libayatana-appindicator-glib` |
| **Install (Arch)** | `libayatana-appindicator` | `libayatana-appindicator-glib` (AUR) |
| **Install (Ubuntu)** | `libayatana-appindicator3-1` | TBD (newer distros) |
| **Header path** | `libayatana-appindicator/` | `libayatana-appindicator-glib/` |

## Breaking API Changes

The GLib version is **NOT** a drop-in replacement:

1. **Menu model**: `GtkMenu` → `GMenu` (completely different API)
2. **Actions**: Must use `GSimpleActionGroup` with named actions
3. **No GTK**: Cannot use any GTK widgets in menu construction
4. **Signal handling**: Actions emit "activate" instead of menu items

## Testing Checklist

- [ ] Tray icon appears on panel
- [ ] Clicking tray icon shows menu
- [ ] Menu items are clickable
- [ ] Menu item clicks invoke Dart callback with correct ID
- [ ] Separators display correctly
- [ ] Disabled items are grayed out
- [ ] Changing menu at runtime works
- [ ] Cleanup on destroy works without crashes

## References

- Source: `/usr/include/libayatana-appindicator-glib/ayatana-appindicator.h`
- Example: `simple-client.c` from libayatana-appindicator-glib
- pkg-config: `ayatana-appindicator-glib` v2.0.3
