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

// Helper to create success response Map
static FlMethodResponse* create_success_response(const gchar* message) {
  g_autoptr(FlValue) response = fl_value_new_map();
  fl_value_set_string_take(response, "success", fl_value_new_bool(true));
  fl_value_set_string_take(response, "message",
                           fl_value_new_string(message));
  return FL_METHOD_RESPONSE(fl_method_success_response_new(response));
}

// Helper to create error response Map with optional code
static FlMethodResponse* create_error_response(const gchar* message,
                                                const gchar* code) {
  g_autoptr(FlValue) response = fl_value_new_map();
  fl_value_set_string_take(response, "success", fl_value_new_bool(false));
  fl_value_set_string_take(response, "message",
                           fl_value_new_string(message));
  if (code != nullptr) {
    fl_value_set_string_take(response, "code", fl_value_new_string(code));
  }
  return FL_METHOD_RESPONSE(fl_method_success_response_new(response));
}

static FlMethodResponse* handle_set_tray_icon(DesktopShellPlugin* self,
                                              FlValue* args) {
  if (self->tray_manager == nullptr) {
    return create_error_response("Tray manager not initialized",
                                 "NOT_INITIALIZED");
  }

  FlValue* icon_path_value = fl_value_lookup_string(args, "iconPath");
  if (icon_path_value == nullptr ||
      fl_value_get_type(icon_path_value) != FL_VALUE_TYPE_STRING) {
    return create_error_response("Missing or invalid iconPath",
                                 "INVALID_ARGS");
  }

  const gchar* icon_path = fl_value_get_string(icon_path_value);
  if (desktop_shell_tray_manager_set_icon(self->tray_manager, icon_path)) {
    return create_success_response("Tray icon set successfully");
  } else {
    return create_error_response("Failed to set tray icon", "SET_FAILED");
  }
}

static FlMethodResponse* handle_set_tray_menu(DesktopShellPlugin* self,
                                              FlValue* args) {
  if (self->tray_manager == nullptr) {
    return create_error_response("Tray manager not initialized",
                                 "NOT_INITIALIZED");
  }

  FlValue* menu_value = fl_value_lookup_string(args, "menu");
  if (menu_value == nullptr ||
      fl_value_get_type(menu_value) != FL_VALUE_TYPE_LIST) {
    return create_error_response("Missing or invalid menu", "INVALID_ARGS");
  }

  if (desktop_shell_tray_manager_set_menu(self->tray_manager, menu_value)) {
    return create_success_response("Tray menu set successfully");
  } else {
    return create_error_response("Failed to set tray menu", "SET_FAILED");
  }
}

static FlMethodResponse* handle_pop_up_tray_menu(DesktopShellPlugin* self) {
  if (self->tray_manager == nullptr) {
    return create_error_response("Tray manager not initialized",
                                 "NOT_INITIALIZED");
  }

  // On Linux, menu appears automatically on tray click
  // This method is mainly for Windows/macOS compatibility
  return create_success_response("Menu handled by system on Linux");
}

static FlMethodResponse* handle_show(DesktopShellPlugin* self) {
  if (self->window_manager == nullptr) {
    return create_error_response("Window manager not initialized",
                                 "NOT_INITIALIZED");
  }

  if (desktop_shell_window_manager_show(self->window_manager)) {
    return create_success_response("Window shown successfully");
  } else {
    return create_error_response("Failed to show window", "SHOW_FAILED");
  }
}

static FlMethodResponse* handle_hide(DesktopShellPlugin* self) {
  if (self->window_manager == nullptr) {
    return create_error_response("Window manager not initialized",
                                 "NOT_INITIALIZED");
  }

  if (desktop_shell_window_manager_hide(self->window_manager)) {
    return create_success_response("Window hidden successfully");
  } else {
    return create_error_response("Failed to hide window", "HIDE_FAILED");
  }
}

static FlMethodResponse* handle_focus(DesktopShellPlugin* self) {
  if (self->window_manager == nullptr) {
    return create_error_response("Window manager not initialized",
                                 "NOT_INITIALIZED");
  }

  if (desktop_shell_window_manager_focus(self->window_manager)) {
    return create_success_response("Window focused successfully");
  } else {
    return create_error_response("Failed to focus window", "FOCUS_FAILED");
  }
}

static FlMethodResponse* handle_set_prevent_close(DesktopShellPlugin* self,
                                                  FlValue* args) {
  if (self->window_manager == nullptr) {
    return create_error_response("Window manager not initialized",
                                 "NOT_INITIALIZED");
  }

  FlValue* prevent_value = fl_value_lookup_string(args, "prevent");
  if (prevent_value == nullptr ||
      fl_value_get_type(prevent_value) != FL_VALUE_TYPE_BOOL) {
    return create_error_response("Missing or invalid prevent flag",
                                 "INVALID_ARGS");
  }

  gboolean prevent = fl_value_get_bool(prevent_value);
  if (desktop_shell_window_manager_set_prevent_close(self->window_manager,
                                                      prevent)) {
    return create_success_response("Prevent close set successfully");
  } else {
    return create_error_response("Failed to set prevent close",
                                 "SET_FAILED");
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
  return create_success_response("Resources destroyed successfully");
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
  } else if (strcmp(method, "popUpTrayMenu") == 0) {
    response = handle_pop_up_tray_menu(self);
  } else if (strcmp(method, "show") == 0) {
    response = handle_show(self);
  } else if (strcmp(method, "hide") == 0) {
    response = handle_hide(self);
  } else if (strcmp(method, "focus") == 0) {
    response = handle_focus(self);
  } else if (strcmp(method, "setPreventClose") == 0) {
    response = handle_set_prevent_close(self, args);
  } else if (strcmp(method, "destroy") == 0) {
    response = handle_destroy(self);
  } else {
    response = FL_METHOD_RESPONSE(
        fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

void desktop_shell_plugin_register_with_registrar(
    FlPluginRegistrar* registrar) {
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
