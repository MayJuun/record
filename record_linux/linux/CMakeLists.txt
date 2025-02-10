cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "record_linux")
project(${PROJECT_NAME} LANGUAGES CXX)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse-simple)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

set(PLUGIN_NAME "record_linux_plugin")

# Define library target
add_library(${PLUGIN_NAME} SHARED
  "record_linux_plugin.cc"
)

# Standard settings
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  CXX_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON
)

# Plugin definitions
target_compile_definitions(${PLUGIN_NAME} PRIVATE 
  FLUTTER_PLUGIN_IMPL
  _GLIBCXX_USE_CXX11_ABI=0
)

# Compiler options
target_compile_options(${PLUGIN_NAME} PRIVATE 
  -Wall
  -Wextra
  -Wno-c99-extensions
  -g0
)

# Include directories, including our mock Flutter headers
target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/testing"  # For our mock headers
  ${PULSE_INCLUDE_DIRS}
  ${GTK3_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PLUGIN_NAME} PRIVATE
  ${PULSE_LIBRARIES}
  ${GTK3_LIBRARIES}
  ${GLIB_LIBRARIES}
)

# Install rules
install(TARGETS ${PLUGIN_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)