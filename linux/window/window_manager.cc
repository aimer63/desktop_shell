#include "window_manager.h"

#include <glib-object.h>
#include <gtk/gtk.h>

// Define the instance struct
struct _DesktopShellWindowManager {
  GObject parent_instance;
  GtkWindow* window;
  gboolean prevent_close;
  gulong delete_event_handler_id;
};

// Define the class struct
typedef struct {
  GObjectClass parent_class;
} DesktopShellWindowManagerClass;

// Define the type
G_DEFINE_TYPE(DesktopShellWindowManager, desktop_shell_window_manager, G_TYPE_OBJECT)

static void desktop_shell_window_manager_init(DesktopShellWindowManager* self) {
  self->window = nullptr;
  self->prevent_close = FALSE;
  self->delete_event_handler_id = 0;
}

static gboolean on_delete_event(GtkWidget* widget,
                                GdkEvent* event,
                                gpointer user_data) {
  DesktopShellWindowManager* self = (DesktopShellWindowManager*)user_data;

  if (self->prevent_close) {
    // Hide window instead of closing
    gtk_widget_hide(GTK_WIDGET(self->window));
    return TRUE;  // Prevent default close
  }

  return FALSE;  // Allow close
}

static void desktop_shell_window_manager_finalize(GObject* object) {
  DesktopShellWindowManager* self = (DesktopShellWindowManager*)object;

  if (self->window != nullptr && self->delete_event_handler_id > 0) {
    g_signal_handler_disconnect(self->window, self->delete_event_handler_id);
  }

  G_OBJECT_CLASS(desktop_shell_window_manager_parent_class)->finalize(object);
}

static void desktop_shell_window_manager_class_init(DesktopShellWindowManagerClass* klass) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = desktop_shell_window_manager_finalize;
}

DesktopShellWindowManager* desktop_shell_window_manager_new(GtkWindow* window) {
  DesktopShellWindowManager* manager = (DesktopShellWindowManager*)g_object_new(
      desktop_shell_window_manager_get_type(), nullptr);

  manager->window = window;

  if (window != nullptr) {
    // Disconnect Flutter's delete-event handler first (Flutter 3.10.1+)
    // See: https://github.com/flutter/engine/pull/40033
    guint handler_id = g_signal_handler_find(
        window, G_SIGNAL_MATCH_ID,
        g_signal_lookup("delete-event", GTK_TYPE_WINDOW),
        0, NULL, NULL, NULL);
    if (handler_id > 0) {
      g_signal_handler_disconnect(window, handler_id);
    }

    // Connect our delete event handler
    manager->delete_event_handler_id = g_signal_connect(
        window, "delete-event", G_CALLBACK(on_delete_event), manager);
  }

  return manager;
}

void desktop_shell_window_manager_destroy(DesktopShellWindowManager* manager) {
  if (manager != nullptr) {
    g_object_unref(manager);
  }
}

gboolean desktop_shell_window_manager_show(DesktopShellWindowManager* manager) {
  if (manager == nullptr || manager->window == nullptr) {
    return FALSE;
  }

  gtk_widget_show(GTK_WIDGET(manager->window));
  gtk_window_present(manager->window);
  return TRUE;
}

gboolean desktop_shell_window_manager_hide(DesktopShellWindowManager* manager) {
  if (manager == nullptr || manager->window == nullptr) {
    return FALSE;
  }

  gtk_widget_hide(GTK_WIDGET(manager->window));
  return TRUE;
}

gboolean desktop_shell_window_manager_focus(DesktopShellWindowManager* manager) {
  if (manager == nullptr || manager->window == nullptr) {
    return FALSE;
  }

  gtk_window_present(manager->window);
  return TRUE;
}

gboolean desktop_shell_window_manager_set_prevent_close(
    DesktopShellWindowManager* manager,
    gboolean prevent) {
  if (manager == nullptr) {
    return FALSE;
  }

  manager->prevent_close = prevent;
  return TRUE;
}
