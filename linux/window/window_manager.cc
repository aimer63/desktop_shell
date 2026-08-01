#include "window_manager.h"

#include <glib-object.h>
#include <gtk/gtk.h>

// Define the instance struct
struct _WindowManager {
  GObject parent_instance;
  GtkWindow* window;
  gboolean prevent_close;
  gulong delete_event_handler_id;
};

// Define the class struct
typedef struct {
  GObjectClass parent_class;
} WindowManagerClass;

// Define the type
G_DEFINE_TYPE(WindowManager, window_manager, G_TYPE_OBJECT)

static void window_manager_init(WindowManager* self) {
  self->window = NULL;
  self->prevent_close = FALSE;
  self->delete_event_handler_id = 0;
}

static gboolean on_delete_event(GtkWidget* widget,
                                GdkEvent* event,
                                gpointer user_data) {
  WindowManager* self = (WindowManager*)user_data;

  if (self->prevent_close) {
    // Hide window instead of closing
    gtk_widget_hide(GTK_WIDGET(self->window));
    return TRUE;  // Prevent default close
  }

  return FALSE;  // Allow close
}

static void window_manager_finalize(GObject* object) {
  WindowManager* self = (WindowManager*)object;

  if (self->window != NULL && self->delete_event_handler_id > 0) {
    g_signal_handler_disconnect(self->window, self->delete_event_handler_id);
  }

  G_OBJECT_CLASS(window_manager_parent_class)->finalize(object);
}

static void window_manager_class_init(WindowManagerClass* klass) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = window_manager_finalize;
}

WindowManager* window_manager_new(GtkWindow* window) {
  WindowManager* manager = (WindowManager*)g_object_new(
      window_manager_get_type(), NULL);

  manager->window = window;

  if (window != NULL) {
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

void window_manager_destroy(WindowManager* manager) {
  if (manager != NULL) {
    g_object_unref(manager);
  }
}

gboolean window_manager_show(WindowManager* manager) {
  if (manager == NULL || manager->window == NULL) {
    return FALSE;
  }

  gtk_widget_show(GTK_WIDGET(manager->window));
  gtk_window_present(manager->window);
  return TRUE;
}

gboolean window_manager_hide(WindowManager* manager) {
  if (manager == NULL || manager->window == NULL) {
    return FALSE;
  }

  gtk_widget_hide(GTK_WIDGET(manager->window));
  return TRUE;
}

gboolean window_manager_focus(WindowManager* manager) {
  if (manager == NULL || manager->window == NULL) {
    return FALSE;
  }

  gtk_window_present(manager->window);
  return TRUE;
}

gboolean window_manager_set_prevent_close(
    WindowManager* manager,
    gboolean prevent) {
  if (manager == NULL) {
    return FALSE;
  }

  manager->prevent_close = prevent;
  return TRUE;
}
