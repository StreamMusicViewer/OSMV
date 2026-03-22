# OSMV - Now Playing Widget for OBS

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![Rust](https://img.shields.io/badge/Language-Rust-brown)
![egui](https://img.shields.io/badge/GUI-egui-lightgrey)

A modern, real-time **"Now Playing"** widget for OBS that displays currently playing music with album artwork and animated transitions. Completely rewritten in **Rust** with **egui** for a premium, lightweight, and portable experience.

## ✨ Features

-   **Discord Rich Presence** — Showcase your music on Discord with automatic album cover lookup (via iTunes API) and status icons (playing/paused/stopped).
-   **Full-Resolution Album Art** — Automatically detects and displays album covers from your media player.
-   **Modern Glassmorphism UI** — A stunning, translucent interface for both the app and the OBS widget.
-   **Real-time Media Polling** — Detects music every second from Spotify, Apple Music, browsers, VLC, and more.
-   **Dynamic Color Support** — Optional feature to match the widget background to the album cover palette.
-   **Single Instance Guard** — Ensures only one instance of the app is running.
-   **Native & Portable** — Single binary executable for Windows and Linux, no complex dependencies required.

## 📂 Repository Structure

```
OSMV/
├── assets/              ← Embedded fonts and icons
├── src-rust/            ← Core Rust implementation
│   ├── media/           ← Media providers (Windows WinRT & Linux MPRIS)
│   ├── app.rs           ← Application logic & background polling
│   ├── discord.rs       ← Discord Rich Presence manager
│   ├── gui.rs           ← egui-based interface
│   ├── main.rs          ← Entry point & single-instance check
│   └── utils.rs         ← Settings and JSON output
├── shared/              ← OBS browser source/widget (HTML/CSS)
│   ├── index.html
│   └── style.css
├── windows/             ← Windows build scripts & resources
├── linux/               ← Linux build scripts
├── Cargo.toml           ← Rust project configuration
└── TROUBLESHOOTING.md   ← Common issues & solutions
```

## 🚀 Quick Start

### Windows
1. Download the latest `OSMV.exe` from the **[Releases](../../releases)** page.
2. Place it in a folder alongside the `shared/` directory.
3. Run `OSMV.exe`.
4. Configure Discord RPC and OBS (see below).

### Linux
1. Download the Linux binary from the **[Releases](../../releases)** page.
2. Ensure you have `libdbus` installed (standard on most distros).
3. `chmod +x osmv && ./osmv`
4. Configure Discord RPC and OBS (see below).

---

## 🎮 Discord Rich Presence Setup

1. Open the **Discord RPC** tab in the app.
2. Enable the feature and enter your **Application Client ID** (from [Discord Developer Portal](https://discord.com/developers/applications)).
3. (Optional) Upload "playing", "paused", and "stopped" icons to your Discord App's Art Assets to see playback status badges.
4. Click **Save Settings**.

---

## 📺 Configure OBS

1. In OBS, add a new **Browser** source.
2. Check **Local file**.
3. Select `shared/index.html`.
4. Set dimensions: **Width: 500**, **Height: 140**.
5. Custom CSS is not required unless you want to override the default styles.

---

## 🛠️ Compiling from Source

You need [Rust](https://rustup.rs/) installed.

### Windows
```powershell
windows\compile_rust.bat
```
*(Produces a release binary and copies it to the root folder)*

### Linux
```bash
linux/build_rust.sh
```

---

## 📄 License
MIT License — free for personal and commercial use.

## 👤 Author
Ulyxx3 (<https://github.com/Ulyxx3>)
