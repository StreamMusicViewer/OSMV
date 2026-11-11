#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

echo "Configuring OSMV C++20 (Release)..."
cmake -B linux/build -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "Building OSMV Daemon + GUI..."
cmake --build linux/build -j"$(nproc)"

echo "Running unit tests..."
ctest --test-dir linux/build --output-on-failure

cp -f linux/build/osmv ./osmv
cp -f linux/build/osmv-gui ./osmv-gui
chmod +x ./osmv ./osmv-gui

echo ""
echo "Done! ./osmv (daemon) and ./osmv-gui are ready."
echo "Required system packages (Arch / Ubuntu):"
echo "  Arch:   sudo pacman -S cmake ninja gcc pkgconf dbus curl glfw-x11 (or glfw-wayland) libgl"
echo "  Ubuntu: sudo apt install cmake ninja-build g++ pkg-config libdbus-1-dev libcurl4-openssl-dev libglfw3-dev libgl-dev"

