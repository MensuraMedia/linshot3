# Architectural Decisions Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2025-11-05 | C + GTK3 for UI framework | Native Linux performance, minimal dependencies, dark theme support |
| 2025-11-05 | Cairo for all drawing/rendering | Already a GTK3 dependency, excellent 2D graphics, annotation-friendly API |
| 2025-11-05 | CMake build system | Standard for C projects, cross-distro compatibility |
| 2026-03-28 | Signal/lock file for single instance | Lightweight IPC without dbus dependency, PrintScreen capture support |
| 2026-03-28 | GList* for annotation storage | GTK-native linked list, simple append/undo, sufficient for annotation counts |
| 2026-03-29 | Per-tool color/width settings | Each annotation tool stores independent settings for better UX |
