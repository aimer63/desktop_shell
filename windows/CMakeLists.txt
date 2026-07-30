cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME "desktop_shell")
project(${PROJECT_NAME} LANGUAGES CXX)

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "desktop_shell_plugin")

add_library(${PLUGIN_NAME} SHARED
  "desktop_shell_plugin.cpp"
  "tray/tray_icon.cpp"
  "tray/tray_icon.h"
  "window/window_manager.cpp"
  "window/window_manager.h"
)

apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE flutter flutter_wrapper_plugin)

# List of absolute paths to libraries that should be bundled with the plugin
set(desktop_shell_bundled_libraries
  ""
  PARENT_SCOPE
)
