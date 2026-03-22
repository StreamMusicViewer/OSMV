#!/bin/bash
# Deploy script for OSMV Rust — Linux
set -e

echo "Building OSMV Rust (release)..."
cargo build --release

echo "Copying binary..."
cp target/release/osmv ./osmv

echo "Done! ./osmv is ready. Requires:"
echo "  - shared/index.html  (OBS Browser Source)"
echo "  - shared/style.css"
echo "  - playerctld running (or any MPRIS-compatible player)"
