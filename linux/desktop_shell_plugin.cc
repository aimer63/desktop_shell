#include "desktop_shell/desktop_shell_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include "tray/tray_manager.h"
#include "window/window_manager.h"

#define DESKTOP_SHELL_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), desktop_shell_plugin_get_type(), \
                              DesktopShellPlugin))

struct _DesktopShellPlugin {
  GObject parent_instance;
  FlPluginRegistrar* registrar;
  FlMethodChannel* channel;
  DesktopShellTrayManager* tray_manager;
  DesktopShellWindowManager* window_manager;
};

G_DEFINE_TYPE(DesktopShellPlugin, desktop_shell_plugin, g_object_get_type())

// Forward declarations
static void desktop_shell_plugin_dispose(GObject* object);

static void desktop_shell_plugin_class_init(DesktopShellPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = desktop_shell_plugin_dispose;
}

static void desktop_shell_plugin_init(DesktopShellPlugin* self) {}

static void desktop_shell_plugin_dispose(GObject* object) {
  DesktopShellPlugin* self = DESKTOP_SHELL_PLUGIN(object);

  if (self->tray_manager != nullptr) {
    desktop_shell_tray_manager_destroy(self->tray_manager);
    self->tray_manager = nullptr;
  }

  if (self->window_manager != nullptr) {
    desktop_shell_window_manager_destroy(self->window_manager);
    self->window_manager = nullptr;
  }

  g_clear_object(&self->channel);
  g_clear_object(&self->registrar);

  G_OBJECT_CLASS(desktop_shell_plugin_parent_class)->dispose(object);
}

static FlMethodResponse* send_error_response(const gchar* message) {
  g_autoptr(FlValue) error_map = fl_value_new_map();
  fl_value_set_string_take(error_map, "error", fl_value_new_bool(true));
  fl_value_set_string_take(error_map, "message", fl_value_new_string(message));
  return FL_METHOD_RESPONSE(fl_method_success_response_new(error_map));
}

static FlMethodResponse* send_success_response() {
  return FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(true)));
}

static FlMethodResponse* handle_set_tray_icon(DesktopShellPlugin* self, FlValue* args) {
  if (self->tray_manager == nullptr) {
    return send_error_response("Tray manager not initialized");
  }

  FlValue* icon_path_value = fl_value_lookup_string(args, "iconPath");
  if (icon_path_value == nullptr || fl_value_get_type(icon_path_value) != FL_VALUE_TYPE_STRING) {
    return send_error_response("Missing or invalid iconPath");
  }

  const gchar* icon_path = fl_value_get_string(icon_path_value);
  if (desktop_shell_tray_manager_set_icon(self->tray_manager, icon_path)) {
    return send_success_response();
  } else {
    return send_error_response("Failed to set tray icon");
  }
}

static FlMethodResponse* handle_set_tray_menu(DesktopShellPlugin* self, FlValue* args) {
  if (self->tray_manager == nullptr) {
    return send_error_response("Tray manager not initialized");
  }

  FlValue* menu_value = fl_value_lookup_string(args, "menu");
  if (menu_value == nullptr || fl_value_get_type(menu_value) != FL_VALUE_TYPE_LIST) {
    return send_error_response("Missing or invalid menu");
  }

  if (desktop_shell_tray_manager_set_menu(self->tray_manager, menu_value)) {
    return send_success_response();
  } else {
    return send_error_response("Failed to set tray menu");
  }
}

static FlMethodResponse* handle_pop_up_context_menu(DesktopShellPlugin* self) {
  if (self->tray_manager == nullptr) {
    return send_error_response("Tray manager not initialized");
  }

  // On Linux, menu appears automatically on tray click
  // This method is mainly for Windows compatibility
  return send_success_response();
}

static FlMethodResponse* handle_show_window(DesktopShellPlugin* self) {
  if (self->window_manager == nullptr) {
    return send_error_response("Window manager not initialized");
  }

  if (desktop_shell_window_manager_show(self->window_manager)) {
    return send_success_response();
  } else {
    return send_error_response("Failed to show window");
  }
}

static FlMethodResponse* handle_hide_window(DesktopShellPlugin* self) {
  if (self->window_manager == nullptr) {
    return send_error_response("Window manager not initialized");
  }

  if (desktop_shell_window_manager_hide(self->window_manager)) {
    return send_success_response();
  } else {
    return send_error_response("Failed to hide window");
  }
}

static FlMethodResponse* handle_focus_window(DesktopShellPlugin* self) {
  if (self->window_manager == nullptr) {
    return send_error_response("Window manager not initialized");
  }

  if (desktop_shell_window_manager_focus(self->window_manager)) {
    return send_success_response();
  } else {
    return send_error_response("Failed to focus window");
  }
}

static FlMethodResponse* handle_set_prevent_close(DesktopShellPlugin* self, FlValue* args) {
  if (self->window_manager == nullptr) {
    return send_error_response("Window manager not initialized");
  }

  FlValue* prevent_value = fl_value_lookup_string(args, "prevent");
  if (prevent_value == nullptr || fl_value_get_type(prevent_value) != FL_VALUE_TYPE_BOOL) {
    return send_error_response("Missing or invalid prevent flag");
  }

  gboolean prevent = fl_value_get_bool(prevent_value);
  if (desktop_shell_window_manager_set_prevent_close(self->window_manager, prevent)) {
    return send_success_response();
  } else {
    return send_error_response("Failed to set prevent close");
  }
}

static FlMethodResponse* handle_destroy(DesktopShellPlugin* self) {
  if (self->tray_manager != nullptr) {
    desktop_shell_tray_manager_destroy(self->tray_manager);
    self->tray_manager = nullptr;
  }
  if (self->window_manager != nullptr) {
    desktop_shell_window_manager_destroy(self->window_manager);
    self->window_manager = nullptr;
  }
  return send_success_response();
}

static void method_call_cb(FlMethodChannel* channel,
                           FlMethodCall* method_call,
                           gpointer user_data) {
  DesktopShellPlugin* self = DESKTOP_SHELL_PLUGIN(user_data);
  const gchar* method = fl_method_call_get_name(method_call);
  FlValue* args = fl_method_call_get_args(method_call);

  g_autoptr(FlMethodResponse) response = nullptr;

  if (strcmp(method, "setTrayIcon") == 0) {
    response = handle_set_tray_icon(self, args);
  } else if (strcmp(method, "setTrayMenu") == 0) {
    response = handle_set_tray_menu(self, args);
  } else if (strcmp(method, "popUpContextMenu") == 0) {
    response = handle_pop_up_context_menu(self);
  } else if (strcmp(method, "showWindow") == 0) {
    response = handle_show_window(self);
  } else if (strcmp(method, "hideWindow") == 0) {
    response = handle_hide_window(self);
  } else if (strcmp(method, "focusWindow") == 0) {
    response = handle_focus_window(self);
  } else if (strcmp(method, "setPreventClose") == 0) {
    response = handle_set_prevent_close(self, args);
  } else if (strcmp(method, "destroy") == 0) {
    response = handle_destroy(self);
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

void desktop_shell_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  DesktopShellPlugin* plugin = DESKTOP_SHELL_PLUGIN(
      g_object_new(desktop_shell_plugin_get_type(), nullptr));

  plugin->registrar = FL_PLUGIN_REGISTRAR(g_object_ref(registrar));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  plugin->channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "desktop_shell",
                            FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(
      plugin->channel, method_call_cb, g_object_ref(plugin),
      g_object_unref);

  // Get the GTK window from the view
  FlView* view = fl_plugin_registrar_get_view(registrar);
  GtkWindow* window = nullptr;
  if (view != nullptr) {
    window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(view)));
  }

  // Initialize tray manager (with channel for callbacks)
  plugin->tray_manager = desktop_shell_tray_manager_new(plugin->channel);

  // Initialize window manager
  plugin->window_manager = desktop_shell_window_manager_new(window);

  g_object_unref(plugin);
}
