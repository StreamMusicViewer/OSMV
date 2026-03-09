#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# compile.sh — Linux build script for OBS Stream Music Viewer v2 (C++ / Qt 6)
# ─────────────────────────────────────────────────────────────────────────────
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$ROOT_DIR"

echo "═══════════════════════════════════════════════════════"
echo " OBS Stream Music Viewer — Linux Build (C++ / Qt 6)"
echo "═══════════════════════════════════════════════════════"
echo

# ── 1. Check CMake ────────────────────────────────────────────────────────────
if ! command -v cmake &>/dev/null; then
    echo "❌ cmake not found."
    if command -v pacman &>/dev/null; then echo "   sudo pacman -S cmake"
    elif command -v apt &>/dev/null;   then echo "   sudo apt install cmake"
    elif command -v dnf &>/dev/null;   then echo "   sudo dnf install cmake"; fi
    exit 1
fi
echo "✔  cmake $(cmake --version | head -1 | awk '{print $3}')"

# ── 2. Check Qt 6 ────────────────────────────────────────────────────────────
if ! pkg-config --exists Qt6Widgets 2>/dev/null && \
   ! qmake6 --version &>/dev/null 2>&1; then
    echo "⚠  Qt 6 not found in pkg-config. Trying anyway..."
    echo "   If build fails, install Qt 6:"
    if command -v pacman &>/dev/null; then echo "   sudo pacman -S qt6-base"
    elif command -v apt &>/dev/null;   then echo "   sudo apt install qt6-base-dev"
    elif command -v dnf &>/dev/null;   then echo "   sudo dnf install qt6-qtbase-devel"; fi
else
    echo "✔  Qt 6 found"
fi

# ── 3. Check playerctl ────────────────────────────────────────────────────────
if ! command -v playerctl &>/dev/null; then
    echo "⚠  playerctl not found (required at runtime)."
    if command -v pacman &>/dev/null; then echo "   sudo pacman -S playerctl"
    elif command -v apt &>/dev/null;   then echo "   sudo apt install playerctl"
    elif command -v dnf &>/dev/null;   then echo "   sudo dnf install playerctl"; fi
else
    echo "✔  playerctl $(playerctl --version)"
fi
echo

# ── 4. Kill any running instance ──────────────────────────────────────────────
if pgrep -x osmv &>/dev/null; then
    echo "Stopping running osmv instance..."
    pkill -x osmv || true
    sleep 1
fi

# ── 5. Configure + Build ──────────────────────────────────────────────────────
echo "Configuring..."
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo
echo "Building..."
cmake --build build --parallel "$(nproc)"

if [ $? -eq 0 ]; then
    echo
    echo "═══════════════════════════════════════════════════════"
    echo "✔  Build successful!"
    echo "   Binary: $(pwd)/build/osmv"
    echo
    echo "   To deploy, copy the following to the same folder:"
    echo "   - build/osmv"
    echo "   - shared/index.html"
    echo "   - shared/style.css"
    echo "   - settings.json (optional)"
    echo "═══════════════════════════════════════════════════════"
else
    echo
    echo "❌ Build failed."
    exit 1
fi
