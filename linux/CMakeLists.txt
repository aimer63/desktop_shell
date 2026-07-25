cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "desktop_shell")
project(${PROJECT_NAME} LANGUAGES CXX)

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "desktop_shell_plugin")

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

add_library(${PLUGIN_NAME} SHARED
  "desktop_shell_plugin.cc"
  "desktop_shell_plugin.h"
  "tray/tray_manager.cc"
  "tray/tray_manager.h"
  "window/window_manager.cc"
  "window/window_manager.h"
)

apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(${PLUGIN_NAME} PRIVATE
  ${DBUS_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  flutter_wrapper_plugin
  ${DBUS_LIBRARIES}
  ${GTK_LIBRARIES}
)

# List of absolute paths to libraries that should be bundled with the plugin
set(desktop_shell_bundled_libraries
  ""
  PARENT_SCOPE
)
