# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

# Project-level configuration.
set(PROJECT_NAME "record_linux")
project(${PROJECT_NAME} LANGUAGES CXX)

# Find the PulseAudio package (libpulse-simple).
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse-simple)

# This value is used when generating builds using this plugin, so it must
# not be changed.
set(PLUGIN_NAME "record_linux_plugin")

# Define the plugin library target. Its name must not be changed.
# Add any new source files that you add to the plugin below.
add_library(${PLUGIN_NAME} SHARED
  "record_linux_plugin.cc"
)

# Apply a standard set of build settings configured in the application-level
# CMakeLists.txt.
apply_standard_settings(${PLUGIN_NAME})

# Symbols are hidden by default. Any symbols that should be exported must
# be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

# Specify include directories and library dependencies.
# We add "${CMAKE_CURRENT_SOURCE_DIR}/include" so that
# #include <record_linux/record_linux_plugin.h> works.
target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  ${PULSE_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_NAME} PRIVATE flutter)
target_link_libraries(${PLUGIN_NAME} PRIVATE ${PULSE_LIBRARIES})

# List of absolute paths to libraries that should be bundled with the plugin.
set(record_linux_bundled_libraries
  ""
  PARENT_SCOPE
)
