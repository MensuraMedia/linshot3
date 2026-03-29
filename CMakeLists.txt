cmake_minimum_required(VERSION 3.10)
project(linshot VERSION 1.2.0 LANGUAGES C)

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
    src/crosshair_drawer.c
    src/sidebar_icons.c
)

# Add header files
set(HEADERS
    include/main_window.h
    include/screen_capture.h
    include/capture_overlay.h
    include/editor_tools.h
    include/screenshot_history.h
    include/utils.h
    include/crosshair_drawer.h
    include/sidebar_icons.h
)

# Create executable
add_executable(linshot ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(linshot
    ${GTK3_LIBRARIES}
    ${X11_LIBRARIES}
    m
)

# Set compiler flags
target_compile_options(linshot PRIVATE
    -Wall
    -Wextra
    -Werror
)

# Copy resources to build directory (for development)
add_custom_command(TARGET linshot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/resources
)

# Install rules
include(GNUInstallDirs)
install(TARGETS linshot DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/linshot.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/icons/linshot-16.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/16x16/apps RENAME linshot.png)
install(FILES resources/icons/linshot-24.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/24x24/apps RENAME linshot.png)
install(FILES resources/icons/linshot-32.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps RENAME linshot.png)
install(FILES resources/icons/linshot-48.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps RENAME linshot.png)
install(FILES resources/icons/linshot-64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME linshot.png)
install(FILES resources/icons/linshot-128.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps RENAME linshot.png)
install(FILES resources/icons/linshot-256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME linshot.png)
