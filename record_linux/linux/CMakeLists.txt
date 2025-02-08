# The Flutter tooling requires that developers have CMake 3.10 or later.
cmake_minimum_required(VERSION 3.10)

# Set the project/plugin name.
set(PROJECT_NAME "record_linux")
project(${PROJECT_NAME} LANGUAGES CXX)

# Find the PulseAudio package (libpulse-simple).
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse-simple)

# This value is used by Flutter tooling; do not change it.
set(PLUGIN_NAME "record_linux_plugin")

# Define the plugin library target. Its name must not be changed.
# IMPORTANT: Only list *.cc or *.cpp files here. Do NOT list .h files.
add_library(${PLUGIN_NAME} SHARED
  "record_linux_plugin.cc"
)

# Apply a standard set of build settings configured in the
# application-level CMakeLists.txt. This comes from the Flutter toolchain.
apply_standard_settings(${PLUGIN_NAME})

# Symbols are hidden by default. Any symbols that should be exported
# must be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden
)

# Let the code know it's part of a Flutter plugin.
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

# (Optional) Silence C99-extension warnings if they appear.
target_compile_options(${PLUGIN_NAME} PRIVATE -Wno-c99-extensions)

# Remove all debug info for this plugin on Linux (no DWARF).
target_compile_options(${PLUGIN_NAME} PRIVATE -g0)

# Tell CMake where to find headers for #include "record_linux_plugin.h"
# Adjust path if your header is in a subdir, e.g. "./include"
target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  ${PULSE_INCLUDE_DIRS}
)

# Link to the Flutter library and PulseAudio libraries
target_link_libraries(${PLUGIN_NAME} PRIVATE flutter)
target_link_libraries(${PLUGIN_NAME} PRIVATE ${PULSE_LIBRARIES})

# Provide an empty variable for bundling if needed.
set(record_linux_bundled_libraries
  ""
  PARENT_SCOPE
)
