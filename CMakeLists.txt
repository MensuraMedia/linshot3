cmake_minimum_required(VERSION 3.10)
project(screenshot_app C)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(X11 REQUIRED x11)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add include directories
include_directories(
    ${GTK3_INCLUDE_DIRS}
    ${X11_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

# Add source files
set(SOURCES
    src/main.c
    src/main_window.c
    src/screen_capture.c
    src/capture_overlay.c
    src/editor_tools.c
    src/screenshot_history.c
    src/utils.c
)

# Add header files
set(HEADERS
    include/main_window.h
    include/screen_capture.h
    include/capture_overlay.h
    include/editor_tools.h
    include/screenshot_history.h
    include/utils.h
)

# Create executable
add_executable(screenshot_app ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(screenshot_app
    ${GTK3_LIBRARIES}
    ${X11_LIBRARIES}
    m
)

# Set compiler flags
target_compile_options(screenshot_app PRIVATE
    -Wall
    -Wextra
    -Werror
)

# Copy resources to build directory
add_custom_command(TARGET screenshot_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/resources
) 