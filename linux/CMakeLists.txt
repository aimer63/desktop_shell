cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "desktop_shell")
project(${PROJECT_NAME} LANGUAGES C CXX)

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "desktop_shell_plugin")

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

# Find appindicator (prefer ayatana, fall back to original)
pkg_check_modules(APPINDICATOR IMPORTED_TARGET ayatana-appindicator3-0.1)
if(APPINDICATOR_FOUND)
  set(HAVE_AYATANA 1)
else()
  pkg_check_modules(APPINDICATOR IMPORTED_TARGET appindicator3-0.1)
endif()

if(NOT APPINDICATOR_FOUND)
  message(
    FATAL_ERROR
    "\n"
    "The desktop_shell package requires ayatana-appindicator3-0.1 or appindicator3-0.1.\n"
    "On Ubuntu/Debian: sudo apt install libappindicator3-dev\n"
    "On Fedora: sudo dnf install libappindicator-gtk3-devel\n"
    "On Arch: sudo pacman -S libappindicator-gtk3"
  )
endif()

add_library(${PLUGIN_NAME} SHARED
  "desktop_shell_plugin.cc"
  "tray/tray_icon.cc"
  "tray/tray_icon.h"
  "tray/tray_menu.cc"
  "tray/tray_menu.h"
  "window/window_manager.cc"
  "window/window_manager.h"
)

set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

if(HAVE_AYATANA)
  target_compile_definitions(${PLUGIN_NAME} PRIVATE HAVE_AYATANA)
endif()

target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(${PLUGIN_NAME} PRIVATE
  ${DBUS_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
  ${APPINDICATOR_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  ${DBUS_LIBRARIES}
  ${GTK_LIBRARIES}
  PkgConfig::APPINDICATOR
)

# List of absolute paths to libraries that should be bundled with the plugin
set(desktop_shell_bundled_libraries
  ""
  PARENT_SCOPE
)
