#!/bin/sh
# Installs a release build into ~/.local/opt/ka3-control and adds an application-menu entry.
# Usage: packaging/install-local.sh <build-dir>   (a Release build directory)
set -e
BUILD=${1:?usage: install-local.sh <release-build-dir>}
HERE=$(cd "$(dirname "$0")" && pwd)
PREFIX="$HOME/.local/opt/ka3-control"

LOG=$(mktemp)
if ! cmake --install "$BUILD" --prefix "$PREFIX" --strip > "$LOG" 2>&1; then
    cat "$LOG"; rm -f "$LOG"; exit 1
fi
rm -f "$LOG"

mkdir -p "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor/scalable/apps"
cp "$HERE/ka3-control.svg" "$HOME/.local/share/icons/hicolor/scalable/apps/ka3-control.svg"
sed -e "s|@EXEC@|$PREFIX/bin/ka3-control|" -e "s|@ICON@|ka3-control|" \
    "$HERE/ka3-control.desktop.in" > "$HOME/.local/share/applications/ka3-control.desktop"
echo "Installed to $PREFIX"
