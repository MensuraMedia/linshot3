# LinShot

LinShot is a modern, open-source screenshot tool built for Linux Debian-based systems. Capture screenshots with real-time area selection, annotate with a full suite of drawing tools, and manage your screenshot history — all from a clean, dark-themed interface.

**Status:** Beta — actively in development with continual updates.
**Created:** January 2025

## Features

- **Area Capture** — Click and drag overlay with visual feedback, crosshair cursor, and live dimension display
- **Annotation Tools** — Line, Arrow, Box, Circle, Text, and Freehand drawing
- **Marquee Selection** — Select and copy regions with dashed marching-ants box
- **Paste & Position** — Ctrl+V pastes images as movable overlays, Flatten to commit
- **Per-Tool Settings** — Independent color palettes and line widths for each tool
- **Text Customization** — Font family (18 fonts), size, bold, and italic options
- **Screenshot History** — Thumbnail grid of all captured screenshots
- **System Tray** — Minimize to tray, right-click menu for quick capture
- **Auto Clipboard** — Every capture is automatically copied to clipboard and saved
- **Sequential Save** — Save dialog defaults to original filename with _1, _2, _3 suffix
- **Keyboard Shortcuts** — Ctrl+S, Ctrl+C, Ctrl+V, Ctrl+Z, Ctrl+N, Ctrl+A, Delete, Escape
- **Modifier Keys** — Shift+drag snaps lines/arrows to 45° angles, Ctrl+drag constrains shapes to squares/circles
- **Configurable Hotkey** — Set as default screenshot app with PrintScreen remapping
- **Single Instance** — Lock file prevents duplicate launches, signals existing instance
- **Dark Theme** — Consistent dark UI across all tabs

## Screenshots

### Main View
![Main View](screenshots/Screenshot1.png?v=2)

### Screenshot History
![Screenshot History](screenshots/Screenshot2.png?v=2)

### Annotation Tools
![Annotation Tools](screenshots/Screenshot3.png?v=2)

## Installation

### Build from source

```bash
# Install dependencies (Debian/Ubuntu/Mint)
sudo apt install cmake build-essential libgtk-3-dev libx11-dev libcairo2-dev

# Build
git clone https://github.com/MensuraMedia/linshot.git
cd linshot
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Run
./linshot
```

### Install .deb package

```bash
# Build the package
bash packaging/build-deb.sh

# Install
sudo dpkg -i linshot_1.0.0_amd64.deb

# Uninstall
sudo apt remove linshot
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New capture |
| Ctrl+S | Save with annotations |
| Ctrl+C | Copy to clipboard (selection or full image) |
| Ctrl+V | Paste from clipboard as movable overlay |
| Ctrl+Z | Undo last annotation |
| Ctrl+A | Select all and copy |
| Shift+Drag | Snap Line/Arrow to 45° angles |
| Ctrl+Drag | Constrain Box/Circle/Select to square/circle |
| Escape | Clear selection or discard paste |
| Delete | Erase content inside selection |
| PrintScreen | Capture (configurable in Settings) |

## License

Creative Commons Attribution-NonCommercial 4.0 (CC BY-NC 4.0).
Free for education, research, and personal projects. Commercial use requires permission.

See [LICENSE](LICENSE) for details.

## Repository

[github.com/MensuraMedia/linshot](https://github.com/MensuraMedia/linshot)
