---
paths:
  - "src/**/*.c"
  - "src/**/*.h"
  - "include/**/*.h"
  - "CMakeLists.txt"
---

# C / GTK3 Best Practices (LinShot-Specific)

> Path-scoped rule — only loads when editing C source, headers, or CMakeLists.txt.

## Memory Safety
- Always bounds-check buffers; prefer `snprintf` over `sprintf`
- Never use unchecked `strcpy` — use `g_strlcpy` or `strncpy` with explicit null termination
- Free all allocated memory; use `g_free` for GLib allocations, `free` for stdlib
- Check return values of all allocation functions (`malloc`, `g_new`, etc.)

## GTK3 Conventions
- Use `g_signal_connect` for all signal connections
- Clean up widget references properly; let GTK manage widget lifecycle where possible
- Use Cairo for all custom drawing — never draw directly to GDK surfaces
- Dark theme: use CSS provider with consistent color tokens

## Code Style (LinShot)
- Indent: 4 spaces, braces on same line
- Functions: `module_verb_noun()` pattern (e.g., `main_window_trigger_capture`)
- Structs/types: PascalCase (e.g., `MainWindow`, `ToolSettings`)
- Enums: `SCREAMING_SNAKE_CASE` (e.g., `TOOL_ARROW`, `TOOL_LINE`)
- Compiler flags: `-Wall -Wextra -Werror` — all warnings are errors

## Build
- CMake minimum 3.10
- Always test with `cmake --build . && ./linshot` before committing
- Add new source files to `CMakeLists.txt` immediately

## Annotations Architecture
- Annotations stored as `GList*` of `Annotation*` pointers
- Cairo rendering in `editor_tools.c`
- Per-tool settings (color, width) in `ToolSettings` struct
