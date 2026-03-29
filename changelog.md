# Project Change Log

> Local record of all changes. Does NOT depend on git. Updated every time a change is made.

| Date-Time | Change Description |
|-----------|-------------------|
| 2025-11-05T14:01:57-05:00 | Initial commit — project created |
| 2025-11-26T11:13:38-05:00 | chore: add real language files from README analysis (SQL, Python, Go, Rust, PostgreSQL, MySQL, Pinecone, etc.) |
| 2026-03-28T18:15:24-04:00 | fix: add missing crosshair_drawer source to CMakeLists and deploy claude agent framework |
| 2026-03-28T20:25:11-04:00 | feat: add sidebar icons, system tray, PrintScreen capture, and default app registration |
| 2026-03-28T22:14:58-04:00 | feat: add .deb packaging, app icon, save sequencing, and shortcut fixes |
| 2026-03-28T22:47:41-04:00 | feat: add marquee selection tool, paste overlay with drag, and flatten button |
| 2026-03-28T23:55:55-04:00 | feat: consistent dark theme, keyboard shortcuts, About page updates, branding |
| 2026-03-29T00:23:01-04:00 | feat: improved arrow, marquee copy fix, settings UX, Ctrl+S support, branding |
| 2026-03-29T00:55:36-04:00 | feat: add Colors tab with per-tool color palettes and text font settings |
| 2026-03-29T01:05:50-04:00 | fix: seamless arrow rendering and reposition Colors tab |
| 2026-03-29T01:20:41-04:00 | feat: add Line tool, per-tool width settings, rename Colors tab to Tools |
| 2026-03-29T01:38:47-04:00 | feat: Line button reordered, Shift-snap for lines/arrows, updated tool defaults |
| 2026-03-29T02:03:03-04:00 | docs: update README with current features, install instructions, and shortcuts |
| 2026-03-29T02:08:40-04:00 | docs: update screenshot with current app UI |
| 2026-03-29T12:00:00-04:00 | chore: initialize universal memory management system — CLAUDE.md, changelog, .claudeignore, .claude/ directory structure |
| 2026-03-29T16:30:00-04:00 | chore: apply full universal standards — add rules (memory-rules, token-hygiene, security), create decisions.md, pending.md, and project-local MEMORY.md index |
| 2026-03-29T16:35:00-04:00 | chore: update settings.local.json to match universal-permissions (added 15 missing tool permissions) |
| 2026-03-29T21:00:00-04:00 | chore: apply full v2026.04 universal standards — hooks (6), skills (5), commands (5), agent-teams (3), routing-rules, settings.json, c-gtk sector rule, CLAUDE.md v2026.04 |
| 2026-03-29T22:00:00-04:00 | feat: multi-paste overlays — Ctrl+V creates independent moveable overlays, all flattened together |
| 2026-03-29T22:30:00-04:00 | feat: Tools tab redesign — circular color swatches with glow, bold titles, shadow with diffused multi-pass rendering, universal settings, 3x3 grid layout |
| 2026-03-29T23:00:00-04:00 | feat: Border tool — decorative double-border frames with color, width, and shadow settings |
| 2026-03-29T23:15:00-04:00 | feat: Blur tool — pixelate/mosaic effect for redacting regions with adjustable block size |
| 2026-03-29T23:30:00-04:00 | feat: Text section layout — vertical flow with font preview (Sample Text + 0123456789), dynamic font/style updates |
| 2026-03-29T23:45:00-04:00 | fix: Universal Setting checkbox — enables/disables controls, color changes now refresh all tool palettes |
| 2026-03-29T23:50:00-04:00 | style: minimal spinbuttons (#ccc +/- chars), restored slider coloring, smaller color circles (18px) |
| 2026-03-30T00:10:00-04:00 | fix: clipboard paste — add gtk_clipboard_store() to all copy paths for X11 persistence |
| 2026-03-30T00:10:00-04:00 | fix: desktop file — update Exec path to linshot binary, add absolute icon path for menu |
| 2026-03-30T00:10:00-04:00 | chore: bump version to 1.2.0 — About page, CMakeLists, desktop files |
| 2026-03-30T00:10:00-04:00 | docs: update About page description with new tools (border, blur, multi-paste, shadows) |
| 2026-03-30T00:30:00-04:00 | fix: universal color now properly refreshes all tool palettes (was overwriting arrow grid) |
| 2026-03-30T00:30:00-04:00 | feat: history page delete — checkbox enables multi-select, delete button removes from disk |
| 2026-03-30T00:30:00-04:00 | fix: save-as dialog defaults to Settings screenshot path instead of original capture directory |
| 2026-03-30T01:00:00-04:00 | fix: Ctrl+S/C/V/Z/N shortcuts now work globally via GDK event filter (not stolen by focused widgets) |
| 2026-03-30T01:00:00-04:00 | fix: universal color now correctly disconnects old handlers and applies across all tool palettes |
| 2026-03-30T01:00:00-04:00 | fix: history delete mode blocks image opening — click only selects for deletion until unchecked |
| 2026-03-30T01:30:00-04:00 | fix: history delete selection now highlights images with blue border on click (toggle select/deselect) |
| 2026-03-30T01:30:00-04:00 | feat: ESC cancels screenshot capture — keyboard grab on overlay popup so ESC is received |
| 2026-03-30T02:00:00-04:00 | fix: history delete — stronger selection highlight (3px blue border + dimmed image), toggle select/deselect |
| 2026-03-30T02:00:00-04:00 | feat: Ctrl+Scroll zoom in/out on screenshot editor (10%-1000%, status bar shows percentage) |
| 2026-03-30T02:30:00-04:00 | refactor: history delete — replace checkbox mode with Ctrl+Click select/deselect, simpler and more reliable |
| 2026-03-30T03:00:00-04:00 | refactor: history selection — native GTK flow box Ctrl+Click/Shift+Click, Delete key, image count, double-click to open |
| 2026-03-30T03:30:00-04:00 | fix: history — remove event_box so flow box receives clicks natively; load ALL image files not just LinShot/Screenshot prefix |
| 2026-03-30T04:00:00-04:00 | fix: Delete key now works for history images; history loads from Settings path not default ~/Pictures |
| 2026-03-30T04:15:00-04:00 | fix: history ghost thumbnails after deletion — reload history data from disk; fix oversized selection highlight |
| 2026-03-30T04:30:00-04:00 | fix: tight thumbnail borders, auto-refresh on tab switch, detect all image files in folder |
| 2026-03-30T05:00:00-04:00 | feat: rename History tab to Files; support all image formats in editor (PNG, JPG, BMP, GIF, WebP, TIFF) |
| 2026-03-30T05:10:00-04:00 | feat: rename Screenshot tab to Image; update all documentation |
| 2026-03-30T05:30:00-04:00 | feat: remove tab outline/curved border; show image dimensions and file size in status bar |
| 2026-03-30T05:45:00-04:00 | feat: dedicated image info panel (left, framed) separate from status bar — persistent, not overwritten |
