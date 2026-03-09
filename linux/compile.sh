#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# compile.sh — Build script for OBS Stream Music Viewer (Linux)
#
# Produces a single self-contained binary using PyInstaller.
# The shared OBS widget files are in ../shared/ (index.html, style.css).
# ─────────────────────────────────────────────────────────────────────────────
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "═══════════════════════════════════════════════════════"
echo " OBS Stream Music Viewer — Linux Build Script"
echo "═══════════════════════════════════════════════════════"
echo

# ── 1. Check Python 3 ────────────────────────────────────────────────────────
if ! command -v python3 &>/dev/null; then
    echo "❌ Python 3 is not installed."
    echo "   Install it with: sudo apt install python3 python3-pip python3-venv"
    exit 1
fi
PYTHON_VER=$(python3 --version)
echo "✔  Found $PYTHON_VER"

# ── 2. Check playerctl (runtime dependency) ──────────────────────────────────
if ! command -v playerctl &>/dev/null; then
    echo "⚠  playerctl is not installed."
    echo "   Install it with: sudo apt install playerctl"
    echo "   (The app will still build, but won't detect music without it.)"
else
    echo "✔  Found $(playerctl --version)"
fi
echo

# ── 3. Create / activate virtual environment ─────────────────────────────────
if [ ! -d ".venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv .venv
fi
source .venv/bin/activate
echo "✔  Virtual environment active: $VIRTUAL_ENV"

# ── 4. Upgrade pip and install requirements ──────────────────────────────────
echo
echo "Installing Python dependencies..."
pip install --upgrade pip --quiet
pip install -r requirements.txt --quiet
pip install pyinstaller --quiet
echo "✔  Dependencies installed."

# ── 5. Kill any running instance before overwriting ─────────────────────────
if pgrep -x osmv &>/dev/null; then
    echo
    echo "Stopping running OSMV instance..."
    pkill -x osmv || true
    sleep 1
fi

# ── 6. Build with PyInstaller ─────────────────────────────────────────────────
echo
echo "Building standalone binary..."
pyinstaller \
    --onefile \
    --name osmv \
    --add-data "discord_rpc_service.py:." \
    osmv.py

if [ $? -eq 0 ]; then
    echo
    echo "═══════════════════════════════════════════════════════"
    echo "✔  Build successful!"
    echo "   Binary: $(pwd)/dist/osmv"
    echo
    echo "   Remember to also copy the OBS widget files:"
    echo "   ../shared/index.html"
    echo "   ../shared/style.css"
    echo "   into the same folder as the binary."
    echo "═══════════════════════════════════════════════════════"
else
    echo
    echo "═══════════════════════════════════════════════════════"
    echo "❌ Build failed. Check the output above."
    echo "═══════════════════════════════════════════════════════"
    exit 1
fi
