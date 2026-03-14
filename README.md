# Now Playing Widget for OBS

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![C++](https://img.shields.io/badge/C++-20-blueviolet)
![Qt](https://img.shields.io/badge/Qt-6-green)

A real-time **"Now Playing"** widget for OBS that displays currently playing music with album artwork and animated transitions. Built in **C++ with Qt 6** — single codebase, runs natively on both **Windows** and **Linux**.

## Features

- **Real-time updates** — Detects currently playing music every second
- **Album artwork** — Displays full-resolution album covers
- **Dynamic color** — Widget background matches the album cover palette
- **Audio visualizer** — Animated bars in OBS (beta)
- **Discord Rich Presence** — Shows what you're listening to on Discord
- **Background operation** — Minimize to system tray
- **Multi-app support** — Spotify, Apple Music, Firefox, Chrome, VLC, and more

## Repository Structure

```
OSMV/
├── src/                 ← C++ source (cross-platform)
│   ├── main.cpp
│   ├── app.cpp / app.h
│   ├── mainwindow.cpp / mainwindow.h   ← Qt 6 UI (same on Win & Linux)
│   ├── mediaprovider.h                 ← Abstract interface
│   ├── mediaprovider_win.cpp           ← Windows: WinRT SMTC
│   ├── mediaprovider_linux.cpp         ← Linux: playerctl / MPRIS2
│   ├── discordrpc.cpp / discordrpc.h
│   └── utils.cpp / utils.h
├── shared/              ← OBS browser source widget (web)
│   ├── index.html
│   └── style.css
├── windows/             ← Windows-specific build & resources
│   ├── OSMV_logo.ico
│   ├── OSMV.rc
│   └── compile.bat
├── linux/               ← Linux-specific build
│   └── compile.sh
├── CMakeLists.txt       ← Build system configuration
├── LICENSE              ← MIT License
├── TROUBLESHOOTING.md   ← Common issues & solutions
└── settings.json        ← Application configuration
```

## How It Works

```
Music Player (Spotify, Apple Music, VLC, browser…)
    ↓
Windows: GlobalSystemMediaTransportControlsSessionManager (WinRT)
Linux:   playerctl + MPRIS2 D-Bus
    ↓
OSMV Qt app (writes current_song.json every ~1s)
    ↓
shared/index.html  (OBS Browser Source, polls the JSON)
    ↓
OBS overlay
```

---

## Quick Start

### Windows

1. Go to the **[Releases](../../releases)** page and download the latest `.zip`.
2. Extract and place `osmv.exe`, `index.html`, and `style.css` in a folder.
3. Double-click `osmv.exe`.
4. Configure OBS (see below).

### Linux

**Install dependencies:**
```bash
sudo pacman -S qt6-base playerctl   # Arch / Manjaro
# or
sudo apt install qt6-base-dev playerctl   # Ubuntu 24.04+
```

1. Go to the **[Releases](../../releases)** page and download the latest Linux binary.
2. Place `osmv`, `index.html`, and `style.css` in the same folder.
3. `chmod +x osmv && ./osmv` — an icon appears in your system tray.
4. Configure OBS (see below).

---

## Configure OBS

1. In OBS, add a new **Browser** source.
2. Check **"Local file"**.
3. Browse and select `index.html` from the folder containing the app.
4. Set dimensions: **Width: 500**, **Height: 140**.
5. Click OK.

*As long as the application is running, your OBS widget updates automatically.*

---

## Compiling from Source

**Requirements (both platforms):** [Qt 6.5+](https://www.qt.io/download), CMake 3.21+

### Linux

```bash
# Arch/Manjaro
sudo pacman -S qt6-base cmake playerctl

# Ubuntu 24.04+
sudo apt install qt6-base-dev cmake playerctl

# Build
./linux/compile.sh
# → binary at build/osmv
```

### Windows

**Requirements:** Qt 6 (MSVC or MinGW), CMake, Visual Studio 2022 or MinGW

```bat
windows\compile.bat
```

The script auto-detects Qt 6 at `C:\Qt\`. Set `QTDIR` manually if needed:
```bat
set QTDIR=C:\Qt\6.7.0\msvc2019_64
windows\compile.bat
```

**Deploy** by placing `osmv.exe` (or `osmv`), `shared/index.html`, and `shared/style.css` in the same folder.

---

## Customization

Edit `shared/style.css` to change the OBS widget appearance:
- Colors and transparency
- Album artwork size
- Animation effects

---

## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

---

## Contributing

Contributions are welcome! Feel free to open issues or submit pull requests.

## License
MIT License — free for personal and commercial use.
