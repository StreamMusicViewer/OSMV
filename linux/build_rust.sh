#!/bin/bash
# Deploy script for OSMV Rust — Linux
set -e

# Always run from the project root
cd "$(dirname "$0")/.."

export CC=clang
export CXX=clang++

echo "Cleaning previous build cache..."
cargo clean

echo "Building OSMV Rust (release)..."
CC=clang CXX=clang++ cargo build --release

echo "Copying binary..."
cp target/release/osmv ./osmv

echo "Done! ./osmv is ready. Requires:"
echo "  - shared/index.html  (OBS Browser Source)"
echo "  - shared/style.css"
echo "  - playerctld running (or any MPRIS-compatible player)"
