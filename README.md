# OSMV (OBS Stream Music Viewer)

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![Platform macOS](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon%20%2F%20Intel)-lightgrey)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C)
![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-blueviolet)

> **OSMV v5.1.0** is an ultra-lightweight native C++20 companion tool for OBS Studio (Now Playing, Discord Rich Presence, and Snaz-inspired modules: Time, Date, Countdown, Chrono Down/Up, System Info, Textline Changer, Dynamic Files, and Twitch Helix).

---

## Ultra-Lightweight C++20 Architecture

OSMV splits the background service from the graphical user interface to keep memory and CPU usage to a minimum while streaming:

1. **Background Daemon (~4.5 to 6 MB private RAM on Windows / zero heavy dependencies on Linux & macOS)**:
   - Monitors media players via WinRT SMTC (Windows), D-Bus MPRIS (Linux), or Apple Music / Spotify via native Cocoa AppleScript (macOS), caching cover art only when the track changes.
   - Updates `shared/current_song.json`, `shared/current_song.js`, and OBS text files only when their content changes (atomic writes).
   - Handles Discord Rich Presence (native IPC / Unix domain socket), HTTP requests (Dynamic Files / Twitch on a dedicated network thread), and the system tray icon (`Shell_NotifyIconW` on Windows, D-Bus `StatusNotifierItem` + `dbusmenu` on Linux, native `NSStatusBar` menu bar item on macOS).
   - Exposes a local IPC server (`\\.\pipe\osmv-ipc-v3` on Windows, `$XDG_RUNTIME_DIR/osmv-ipc-v3.sock` on Linux, `/tmp/osmv-ipc-v3-<uid>.sock` on macOS) to communicate with the GUI.
2. **Dear ImGui Interface (DirectX 11 on Windows, GLFW + OpenGL 3 on Linux & macOS)**:
   - Event-driven rendering (~0% idle CPU), Dark/Light themes, 5 accent colors, EN/FR bilingual UI, placeholder insertion chips, and one-click OBS path copy buttons.
   - On Windows, graphics DLLs (`d3d11.dll`, `dxgi.dll`, `dwmapi.dll`) are `/DELAYLOAD`-linked and loaded only when the GUI opens.
   - Closing the window `[X]` or clicking **Minimize to Tray** terminates the GUI process and frees all of its RAM (~51 to 58 MB on Windows) while leaving the daemon active.

---

## Features

- **Now Playing & OBS Overlay Studio**: Real-time OBS Browser widget (`shared/index.html`) with embedded **Live OBS Stage Preview**, standalone `shared/preview.html` (**Browser Preview**), 7 built-in presets (`classic`, `glass`, `neon`, `compact`, `vinyl`, `minimal`, `banner`), **Saved Presets (Multi-Scene OBS)** (`?preset=<Name>`), layout/typography/token controls (`$song`, `$artist`, `$album`, `$player`, `$status`, `*_upper`), dynamic album cover tinting, 5-style audio visualizer, and a syntax-highlighted auto-sizing CSS code editor. Also exports Snaz-compatible text/JSON files (`TrackArtist.txt`, `TrackSong.txt`, `TrackAlbum.txt`, `PlayingNow.txt`, and `track-info.json`).
- **Time & Date**: `Time.txt` (`$h`, `$m`, `$s`, `$tt`, 12h/24h) and `Date.txt` (`$dd-MM-yy`, `$dd-MM-yyyy`, `$dd MMMM yyyy`, `$dddd dd MMMM yyyy`, localized EN/FR `$date`).
- **Timers & Chronos**: `Countdown.txt` toward a target date/time (`$d`, `$h`, `$m`, `$s`), `ChronoDown.txt` (countdown timer with Start/Pause/Reset), and `ChronoUp.txt` (stopwatch with `$totalminutes`).
- **System Info**: `SystemInfo.txt` (`$CPU_USAGE`, `$RAM_MB_AVAIL`, `$RAM_%_IN_USE`, `$RAM_%_AVAIL`, `$AMOUNT_PROCESSES`, `$UPLOAD`, `$DOWNLOAD`) via native Windows APIs, Linux `/proc`, and macOS Mach kernel (`host_statistics`, `sysctl`).
- **Textline Changer & Dynamic Files**: sequential or random message rotation (`TextlineChanger.txt`) and periodic HTTP/HTTPS fetching of remote text or image files with custom headers.
- **Discord Rich Presence**: Listening mode (`type: 2`, album shown without repeating the artist), automatic iTunes cover lookup (`- Topic` suffix cleanup and bounded cache), `SET_ACTIVITY` error detection, and automatic reconnection.
- **Twitch (Helix API)**: automatic localhost OAuth token capture (port 80 callback server with manual URL fallback), live viewer counter (`TwitchViewers.txt` via `$viewers`), category search, and stream title/game updates with favorite presets.
- **Settings & Telemetry**: customizable output directory, launch on system startup (`--silent`), and live RAM monitoring for both daemon and GUI.

---

## Installation & Quick Start

### macOS (Apple Silicon M1/M2/M3/M4 & Intel)

#### 1. Download & Extract
Download the latest `OSMV_vX.Y.Z_macos.tar.gz` from the [Releases](https://github.com/StreamMusicViewer/OSMV/releases) page.

Open Terminal and navigate to your download folder:
```bash
# Extract the archive
tar -xzf OSMV_*_macos.tar.gz
cd OSMV_*_macos
```

#### 2. Gatekeeper Authorization (First Launch)
Because the binary is downloaded from GitHub, macOS may apply a quarantine flag. Clear it with:
```bash
xattr -cr .
```

#### 3. Run OSMV
Launch the graphical interface:
```bash
./osmv-gui
```
- **Automatic Daemon**: Starting `osmv-gui` automatically launches the background daemon (`osmv`).
- **Top Menu Bar Icon**: An `OSMV` status icon will appear in your macOS top menu bar. You can click it anytime to reopen the window or quit.
- **Media Support**: Automatically detects whatever is currently playing in **Apple Music** or **Spotify** without any configuration.

---

### Windows (10 / 11 64-bit)
1. Download `OSMV_vX.Y.Z_win.zip` from [Releases](https://github.com/StreamMusicViewer/OSMV/releases).
2. Extract the archive into a folder of your choice (e.g. `C:\Tools\OSMV`).
3. Double-click `OSMV.exe` to run.

---

### Linux (Ubuntu, Debian, Fedora, Arch)
1. Download `OSMV_vX.Y.Z_linux.tar.gz` from [Releases](https://github.com/StreamMusicViewer/OSMV/releases).
2. Extract and launch:
   ```bash
   tar -xzf OSMV_*_linux.tar.gz
   cd OSMV_*_linux
   ./osmv-gui
   ```

---

## Building from Source

### macOS
**Prerequisites**:
- Xcode Command Line Tools: `xcode-select --install`
- Homebrew packages: `brew install cmake ninja glfw`

```bash
# Configure with Ninja and Homebrew search path
cmake -B macos/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/opt/homebrew;/usr/local"

# Build daemon and GUI
cmake --build macos/build -j

# Run tests
ctest --test-dir macos/build --output-on-failure

# Launch the app
./macos/build/osmv-gui
```

### Windows (no external DLLs required)
- **Visual Studio 2022/2026 Build Tools** (C++ x64 + Windows 10/11 SDK)
- **CMake 3.20+** and **Ninja**

```bat
windows\build.bat
```
Builds cleanly from scratch (`--clean-first`) in Release mode, runs unit tests (`osmv_unit_tests`), and copies standalone `OSMV.exe` to the project root.

### Linux
```bash
# Arch Linux (GLFW 3.4+ Wayland + X11)
sudo pacman -S cmake ninja gcc pkgconf dbus curl glfw libgl

# Ubuntu / Debian
sudo apt install cmake ninja-build g++ pkg-config libdbus-1-dev libcurl4-openssl-dev libglfw3-dev libgl-dev

./linux/build.sh
```
Compiles and copies `./osmv` (daemon) and `./osmv-gui` (GUI) to the project root. On Wayland/Sway, the window exposes `app_id="io.github.streammusicviewer.OSMV"`, and on X11/XWayland `WM_CLASS` (`instance="osmv"`, `class="OSMV"`). A `.desktop` entry and icon are also provided in `linux/` (installed via `cmake --install linux/build`).

---

## OBS Usage

1. **Music Widget (Single Style)**: **Browser** source -> **Local file** -> select `shared/index.html` (Width `500`, Height `140`, or larger if positioned using Studio canvas margins).
2. **Music Widget (Multi-Scene Presets)**: Save presets in **Now Playing -> Presets & Style -> Saved Presets (Multi-Scene OBS)**, click **Scene URL** (`file:///.../shared/index.html?preset=<Name>`), add an OBS **Browser** source with **Local file** unchecked, and paste the copied URL.
3. **Dynamic Text**: **Text (GDI+)** / **Text (FreeType 2)** source -> *Read from file* -> select the generated files in `shared/dynamic_text/` (or your custom directory).
4. **Dynamic Images**: **Image** source -> select the image file saved by **Dynamic Files**.

---

## Documentation

Read the full documentation in `docs/` or at [streammusicviewer.github.io/OSMV](https://streammusicviewer.github.io/OSMV/). See [CHANGELOG.md](CHANGELOG.md) for release history, or check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) if you run into issues.

## License
GNU General Public License v3.0 (GPL-3.0) — See [LICENSE](LICENSE) for details.
