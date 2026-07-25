#include "desktop_shell_plugin.h"

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
static void method_call_cb(FlMethodChannel* channel,
                           FlMethodCall* method_call,
                           gpointer user_data);

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

static void send_error_result(FlMethodResult* result, const gchar* message) {
  g_autoptr(FlValue) error_map = fl_value_new_map();
  fl_value_set_string_take(error_map, "error", fl_value_new_bool(true));
  fl_value_set_string_take(error_map, "message", fl_value_new_string(message));
  fl_method_success_response(result, error_map);
}

static void send_success_result(FlMethodResult* result) {
  fl_method_success_response(result, fl_value_new_bool(true));
}

static void method_call_cb(FlMethodChannel* channel,
                           FlMethodCall* method_call,
                           gpointer user_data) {
  DesktopShellPlugin* self = DESKTOP_SHELL_PLUGIN(user_data);
  const gchar* method = fl_method_call_get_name(method_call);
  FlValue* args = fl_method_call_get_args(method_call);
  g_autoptr(FlMethodResult) result = fl_method_call_get_result_object(method_call);

  if (strcmp(method, "setTrayIcon") == 0) {
    if (self->tray_manager == nullptr) {
      send_error_result(result, "Tray manager not initialized");
      return;
    }

    FlValue* icon_path_value = fl_value_lookup_string(args, "iconPath");
    if (icon_path_value == nullptr || fl_value_get_type(icon_path_value) != FL_VALUE_TYPE_STRING) {
      send_error_result(result, "Missing or invalid iconPath");
      return;
    }

    const gchar* icon_path = fl_value_get_string(icon_path_value);
    if (desktop_shell_tray_manager_set_icon(self->tray_manager, icon_path)) {
      send_success_result(result);
    } else {
      send_error_result(result, "Failed to set tray icon");
    }

  } else if (strcmp(method, "setTrayMenu") == 0) {
    if (self->tray_manager == nullptr) {
      send_error_result(result, "Tray manager not initialized");
      return;
    }

    FlValue* menu_value = fl_value_lookup_string(args, "menu");
    if (menu_value == nullptr || fl_value_get_type(menu_value) != FL_VALUE_TYPE_LIST) {
      send_error_result(result, "Missing or invalid menu");
      return;
    }

    if (desktop_shell_tray_manager_set_menu(self->tray_manager, menu_value, self->channel)) {
      send_success_result(result);
    } else {
      send_error_result(result, "Failed to set tray menu");
    }

  } else if (strcmp(method, "popUpContextMenu") == 0) {
    if (self->tray_manager == nullptr) {
      send_error_result(result, "Tray manager not initialized");
      return;
    }

    // On Linux, menu appears automatically on tray click
    // This method is mainly for Windows compatibility
    send_success_result(result);

  } else if (strcmp(method, "showWindow") == 0) {
    if (self->window_manager == nullptr) {
      send_error_result(result, "Window manager not initialized");
      return;
    }

    if (desktop_shell_window_manager_show(self->window_manager)) {
      send_success_result(result);
    } else {
      send_error_result(result, "Failed to show window");
    }

  } else if (strcmp(method, "hideWindow") == 0) {
    if (self->window_manager == nullptr) {
      send_error_result(result, "Window manager not initialized");
      return;
    }

    if (desktop_shell_window_manager_hide(self->window_manager)) {
      send_success_result(result);
    } else {
      send_error_result(result, "Failed to hide window");
    }

  } else if (strcmp(method, "focusWindow") == 0) {
    if (self->window_manager == nullptr) {
      send_error_result(result, "Window manager not initialized");
      return;
    }

    if (desktop_shell_window_manager_focus(self->window_manager)) {
      send_success_result(result);
    } else {
      send_error_result(result, "Failed to focus window");
    }

  } else if (strcmp(method, "setPreventClose") == 0) {
    if (self->window_manager == nullptr) {
      send_error_result(result, "Window manager not initialized");
      return;
    }

    FlValue* prevent_value = fl_value_lookup_string(args, "prevent");
    if (prevent_value == nullptr || fl_value_get_type(prevent_value) != FL_VALUE_TYPE_BOOL) {
      send_error_result(result, "Missing or invalid prevent flag");
      return;
    }

    gboolean prevent = fl_value_get_bool(prevent_value);
    if (desktop_shell_window_manager_set_prevent_close(self->window_manager, prevent)) {
      send_success_result(result);
    } else {
      send_error_result(result, "Failed to set prevent close");
    }

  } else if (strcmp(method, "destroy") == 0) {
    if (self->tray_manager != nullptr) {
      desktop_shell_tray_manager_destroy(self->tray_manager);
      self->tray_manager = nullptr;
    }
    if (self->window_manager != nullptr) {
      desktop_shell_window_manager_destroy(self->window_manager);
      self->window_manager = nullptr;
    }
    send_success_result(result);

  } else {
    fl_method_not_implemented_response(result);
  }
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
      plugin->channel, method_call_cb, plugin, nullptr);

  // Get the GTK window
  GtkWindow* window = GTK_WINDOW(fl_plugin_registrar_get_view(registrar));

  // Initialize tray manager (using StatusNotifierItem)
  plugin->tray_manager = desktop_shell_tray_manager_new();
  if (plugin->tray_manager != nullptr) {
    desktop_shell_tray_manager_initialize(plugin->tray_manager);
  }

  // Initialize window manager
  plugin->window_manager = desktop_shell_window_manager_new(window);

  g_object_unref(plugin);
}
