#include "tray_icon.h"
#include "tray_menu.h"

#include <flutter_linux/flutter_linux.h>
#include <libayatana-appindicator-glib/ayatana-appindicator.h>

struct _TrayIcon {
  GObject parent_instance;
  FlMethodChannel* channel;
  AppIndicator* indicator;
  GMenu* menu;
  GSimpleActionGroup* actions;
};

G_DEFINE_TYPE(TrayIcon, tray_icon, G_TYPE_OBJECT)

static void tray_icon_dispose(GObject* object) {
  TrayIcon* self = TRAY_ICON(object);

  if (self->indicator != nullptr) {
    app_indicator_set_status(self->indicator, APP_INDICATOR_STATUS_PASSIVE);
    g_clear_object(&self->indicator);
  }

  if (self->menu != nullptr) {
    g_object_unref(self->menu);
    self->menu = nullptr;
  }

  if (self->actions != nullptr) {
    g_object_unref(self->actions);
    self->actions = nullptr;
  }

  g_clear_object(&self->channel);

  G_OBJECT_CLASS(tray_icon_parent_class)->dispose(object);
}

static void tray_icon_class_init(TrayIconClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = tray_icon_dispose;
}

static void tray_icon_init(TrayIcon* self) {
  self->channel = nullptr;
  self->indicator = nullptr;
  self->menu = nullptr;
  self->actions = nullptr;
}

TrayIcon* tray_icon_new(FlMethodChannel* channel) {
  TrayIcon* self = TRAY_ICON(g_object_new(tray_icon_get_type(), nullptr));
  self->channel = FL_METHOD_CHANNEL(g_object_ref(channel));
  return self;
}

void tray_icon_destroy(TrayIcon* self) {
  if (self != nullptr) {
    g_object_unref(self);
  }
}

gboolean tray_icon_set_icon(TrayIcon* self, const gchar* icon_path) {
  g_print("[TRAY] tray_icon_set_icon called with path: %s\n", icon_path ? icon_path : "(null)");
  
  if (self == nullptr) {
    g_print("[TRAY] ERROR: self is null\n");
    return FALSE;
  }

  // Create indicator if not exists
  if (self->indicator == nullptr) {
    g_print("[TRAY] Creating new AppIndicator\n");
    self->indicator = app_indicator_new("desktop_shell", icon_path,
                                        APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    g_print("[TRAY] AppIndicator created at %p\n", (void*)self->indicator);
    // Menu and actions will be set later in tray_icon_set_menu()
  }

  app_indicator_set_status(self->indicator, APP_INDICATOR_STATUS_ACTIVE);
  app_indicator_set_icon(self->indicator, icon_path, "");
  g_print("[TRAY] Indicator status set to ACTIVE\n");

  return TRUE;
}

gboolean tray_icon_set_menu(TrayIcon* self, FlValue* menu_items) {
  g_print("[TRAY] tray_icon_set_menu called\n");
  
  if (self == nullptr) {
    g_print("[TRAY] ERROR: self is null\n");
    return FALSE;
  }

  g_print("[TRAY] Current indicator: %p\n", (void*)self->indicator);
  g_print("[TRAY] Current menu: %p\n", (void*)self->menu);
  g_print("[TRAY] Current actions: %p\n", (void*)self->actions);

  // Clean up old menu and actions
  if (self->menu != nullptr) {
    g_print("[TRAY] Unreffing old menu\n");
    g_object_unref(self->menu);
    self->menu = nullptr;
  }

  if (self->actions != nullptr) {
    g_print("[TRAY] Unreffing old actions\n");
    g_object_unref(self->actions);
    self->actions = nullptr;
  }

  // Build new menu and actions using tray_menu module
  // This sets self->menu and self->actions
  g_print("[TRAY] Building new menu...\n");
  tray_menu_build(self, menu_items);
  g_print("[TRAY] After build - menu: %p, actions: %p\n", (void*)self->menu, (void*)self->actions);

  // Set menu and actions on indicator AFTER building
  if (self->indicator != nullptr) {
    g_print("[TRAY] Setting menu on indicator...\n");
    app_indicator_set_menu(self->indicator, self->menu);
    g_print("[TRAY] Setting actions on indicator...\n");
    app_indicator_set_actions(self->indicator, self->actions);
    g_print("[TRAY] Menu and actions set on indicator\n");
  } else {
    g_print("[TRAY] ERROR: No indicator to set menu on!\n");
  }

  return TRUE;
}

FlMethodChannel* tray_icon_get_channel(TrayIcon* self) {
  return self != nullptr ? self->channel : nullptr;
}

GMenu* tray_icon_get_menu(TrayIcon* self) {
  return self != nullptr ? self->menu : nullptr;
}

void tray_icon_set_menu_model(TrayIcon* self, GMenu* menu) {
  if (self != nullptr) {
    if (self->menu != nullptr) {
      g_object_unref(self->menu);
    }
    self->menu = menu != nullptr ? G_MENU(g_object_ref(menu)) : nullptr;
  }
}

GSimpleActionGroup* tray_icon_get_actions(TrayIcon* self) {
  return self != nullptr ? self->actions : nullptr;
}

void tray_icon_set_actions_group(TrayIcon* self, GSimpleActionGroup* actions) {
  if (self != nullptr) {
    if (self->actions != nullptr) {
      g_object_unref(self->actions);
    }
    self->actions = actions != nullptr ? G_SIMPLE_ACTION_GROUP(g_object_ref(actions)) : nullptr;
  }
}
