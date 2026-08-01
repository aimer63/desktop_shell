#include "tray_icon.h"
#include "tray_menu.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#ifdef HAVE_AYATANA
#include <libayatana-appindicator/app-indicator.h>
#else
#include <libappindicator/app-indicator.h>
#endif

struct _TrayIcon {
  GObject parent_instance;
  FlMethodChannel* channel;
  AppIndicator* indicator;
  GtkWidget* menu;
};

G_DEFINE_TYPE(TrayIcon, tray_icon, G_TYPE_OBJECT)

static void tray_icon_dispose(GObject* object) {
  TrayIcon* self = TRAY_ICON(object);

  if (self->indicator != nullptr) {
    app_indicator_set_status(self->indicator, APP_INDICATOR_STATUS_PASSIVE);
    g_clear_object(&self->indicator);
  }

  if (self->menu != nullptr && GTK_IS_WIDGET(self->menu)) {
    gtk_widget_destroy(self->menu);
    self->menu = nullptr;
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
  if (self == nullptr) {
    return FALSE;
  }

  // Create indicator if not exists
  if (self->indicator == nullptr) {
    // The library marks app_indicator_new() as deprecated but provides no
    // alternative constructor. This is the only way to create an AppIndicator.
    // Suppress the deprecation warning since we have no choice.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    self->indicator = app_indicator_new("desktop_shell", icon_path,
                                        APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
#pragma GCC diagnostic pop

    // Create empty menu initially
    if (self->menu == nullptr) {
      self->menu = gtk_menu_new();
      gtk_widget_show(self->menu);
    }

    app_indicator_set_menu(self->indicator, GTK_MENU(self->menu));
  }

  app_indicator_set_status(self->indicator, APP_INDICATOR_STATUS_ACTIVE);
  app_indicator_set_icon_full(self->indicator, icon_path, "");

  return TRUE;
}

gboolean tray_icon_set_menu(TrayIcon* self, FlValue* menu_items) {
  if (self == nullptr) {
    return FALSE;
  }

  // Destroy old menu
  if (self->menu != nullptr) {
    gtk_widget_destroy(self->menu);
    self->menu = nullptr;
  }

  // Build new menu using tray_menu module
  self->menu = tray_menu_build(self, menu_items);

  if (self->indicator != nullptr) {
    app_indicator_set_menu(self->indicator, GTK_MENU(self->menu));
  }

  return TRUE;
}

FlMethodChannel* tray_icon_get_channel(TrayIcon* self) {
  return self != nullptr ? self->channel : nullptr;
}
