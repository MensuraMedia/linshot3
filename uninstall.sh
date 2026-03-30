#!/bin/bash
# LinShot Uninstaller
set -e

INSTALL_DIR="$HOME/.local/share/linshot"
BIN_DIR="$HOME/.local/bin"
APP_DIR="$HOME/.local/share/applications"

echo ""
echo "  Uninstalling LinShot..."
echo ""

rm -f "$BIN_DIR/linshot"
rm -f "$APP_DIR/linshot.desktop"
rm -rf "$INSTALL_DIR"
update-desktop-database "$APP_DIR" 2>/dev/null || true

echo "  LinShot removed."
echo ""

read -rp "  Remove settings too? (~/.config/linshot) [y/N] " answer
if [[ "${answer,,}" == "y" ]]; then
    rm -rf "$HOME/.config/linshot"
    echo "  Settings removed."
fi

echo "  Done."
echo ""
