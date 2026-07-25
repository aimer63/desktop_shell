//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <desktop_shell/desktop_shell_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) desktop_shell_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "DesktopShellPlugin");
  desktop_shell_plugin_register_with_registrar(desktop_shell_registrar);
}
