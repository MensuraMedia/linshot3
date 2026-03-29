# Project: LinShot — CLAUDE.md (v2026.04 — Claude Code Native)

## Overview
LinShot is a Linux screenshot capture and annotation tool built in C with GTK3 — a native, lightweight alternative to ShareX/Greenshot for Debian-based Linux systems.

## Architecture
- Language/Framework: C (C11 standard) + GTK3
- Build system: CMake (minimum 3.10)
- Key dependencies: GTK3 (gtk+-3.0), X11, Cairo, GDK-Pixbuf, libm

## Build & Runtime Standards (Enforced)
```bash
# Install dependencies (Debian/Ubuntu/Mint)
sudo apt install cmake build-essential libgtk-3-dev libx11-dev libcairo2-dev

# Build
mkdir -p build && cd build && cmake .. && make

# Build (Release)
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Run
./build/linshot

# Package (.deb)
bash packaging/build-deb.sh
```
- Use /plan-first for complex features or multi-file changes
- Use /build-test to run the full pipeline
- Compiler flags: `-Wall -Wextra -Werror` — all warnings are errors

## Project Conventions

### File Organization
- `src/` — All .c source files
- `include/` — All .h header files
- `resources/` — Icons, images, and other assets
- `packaging/` — .deb packaging scripts and desktop file
- `build/` — CMake build output (git-ignored)

### Naming Conventions
- Files: `snake_case.c` / `snake_case.h`
- Functions: `module_verb_noun()` pattern (e.g., `main_window_trigger_capture`, `annotation_create`)
- Structs/types: `PascalCase` (e.g., `MainWindow`, `ToolSettings`, `AnnotationType`)
- Enums: `SCREAMING_SNAKE_CASE` for values (e.g., `TOOL_ARROW`, `TOOL_LINE`)
- Constants/macros: `SCREAMING_SNAKE_CASE`
- Local variables and struct fields: `snake_case`

### Header Guards
All headers use `#ifndef MODULE_NAME_H` / `#define MODULE_NAME_H` / `#endif // MODULE_NAME_H`

### Include Order (in .c files)
1. Corresponding header (`../include/module.h`)
2. Standard library headers
3. GTK/system headers

### Code Style
- Indent: 4 spaces
- Braces: same line for functions and control flow (`{` at end of line)
- Comments: `//` for inline, block comments for section headers

### Architecture Patterns
- `MainWindow` struct holds GTK widget pointers (the top-level application state)
- `MainWindowData` extends `MainWindow` with canvas/editor state
- Annotations are stored as a `GList*` of `Annotation*` pointers
- Cairo is used for all drawing (canvas and annotation rendering)
- Signal/lock file approach for single-instance and PrintScreen capture

### Key Modules
| File | Responsibility |
|------|---------------|
| `main.c` | Entry point, single-instance lock, SIGUSR1 handler |
| `main_window.c` | Main GTK window, tabs, tool switching, keyboard shortcuts |
| `screen_capture.c` | Screenshot capture using GDK/X11 |
| `capture_overlay.c` | Fullscreen overlay for area selection |
| `editor_tools.c` | Annotation types, drawing logic (Cairo) |
| `screenshot_history.c` | Files tab, thumbnail management, image discovery |
| `sidebar_icons.c` | Sidebar icon rendering |
| `crosshair_drawer.c` | Crosshair cursor for capture overlay |
| `utils.c` | Shared utility functions |

## Sector-Specific Rules
<!-- Path-scoped rules load automatically when editing matching files -->
@.claude/rules/ for all active rules (includes C/GTK rule: `.claude/rules/c-gtk.md`)

## Memory & Workflow
- Use official Auto Memory (/memory) for Claude's own learnings across sessions
- Human-readable history supplements Auto Memory:
  - Session logs: `.claude/memory/sessions/`
  - Change manifests: `.claude/memory/changes/`
  - Decision log: `.claude/memory/decisions.md`
  - Pending items: `.claude/memory/pending.md`
  - Memory index: `.claude/memory/MEMORY.md`
- End every significant session with /session-end or the session-end checklist
- Update changelog.md as changes are made, not after

## Hooks (Automated)
Lifecycle hooks are configured in `.claude/settings.json`:
- **SessionStart**: Injects pending items, last session log, and recent changelog
- **PreToolUse**: Security gate blocks secret file access and destructive commands
- **PostToolUse**: Auto-format C files with clang-format after edits
- **InstructionsLoaded**: Reports rule file count
- **SubagentStart/Stop**: Logs agent activity to board.md
- **TeammateIdle**: Prompts for pending work
- **TaskCompleted**: Quality gates on completion

## Custom Commands / Skills
- `/plan-first` — Plan complex tasks before executing
- `/build-test` — Run full build + test pipeline from CLAUDE.md
- `/session-end` — End-of-session wrap-up and logging
- `/route` — Score task intensity and recommend agent + model
- `/team` — Launch coordinated multi-agent workflow

## References
- @README.md for user-facing feature documentation
- @.claude/rules/ for path-scoped rules (auto-activate per file type)
- @.claude/memory/decisions.md for architectural decision history
- @.claude/agent-teams/ for team orchestration templates
- @.claude/routing-rules.md for intensity-based model selection
