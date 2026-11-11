# OSMV (OBS Stream Music Viewer)

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C)
![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-blueviolet)

> **OSMV v5.0.0** is an ultra-lightweight native C++20 companion tool for OBS Studio (Now Playing, Discord Rich Presence, and Snaz-inspired modules: Time, Date, Countdown, Chrono Down/Up, System Info, Textline Changer, Dynamic Files, and Twitch Helix).

---

## Ultra-Lightweight C++20 Architecture

OSMV splits the background service from the graphical user interface to keep memory and CPU usage to a minimum while streaming:

1. **Background Daemon (~4.5 to 6 MB private RAM on Windows / zero X11, Wayland, OpenGL, or GTK dependencies on Linux)**:
   - Monitors media players via WinRT SMTC (Windows) or D-Bus MPRIS (Linux), caching cover art only when the track changes.
   - Updates `shared/current_song.json` and OBS text files only when their content changes (atomic writes).
   - Handles Discord Rich Presence (native IPC), HTTP requests (Dynamic Files / Twitch on a dedicated network thread), and the system tray icon (`Shell_NotifyIconW` on Windows, D-Bus `StatusNotifierItem` + `dbusmenu` on Linux).
   - Exposes a local IPC server (`\\.\pipe\osmv-ipc-v3` on Windows, `$XDG_RUNTIME_DIR/osmv-ipc-v3.sock` on Linux) to communicate with the GUI.
2. **Dear ImGui Interface (DirectX 11 on Windows, GLFW + OpenGL 3 on Linux)**:
   - Event-driven rendering (~0% idle CPU), Dark/Light themes, 5 accent colors, EN/FR bilingual UI, placeholder insertion chips, and one-click OBS path copy buttons.
   - On Windows, graphics DLLs (`d3d11.dll`, `dxgi.dll`, `dwmapi.dll`) are `/DELAYLOAD`-linked and loaded only when the GUI opens.
   - Closing the window `[X]` or clicking **Minimize to Tray** terminates the GUI process and frees all of its RAM (~51 to 58 MB on Windows) while leaving the daemon active.

---

## Features

- **Now Playing**: OBS Browser widget `shared/index.html` (cover art, dynamic color, audio visualizer) + Snaz-compatible text/JSON exports (`TrackArtist.txt`, `TrackSong.txt`, `TrackAlbum.txt`, `PlayingNow.txt` with `$artist`, `$song`, `$album`, `*_upper`, and `track-info.json`).
- **Time & Date**: `Time.txt` (`$h`, `$m`, `$s`, `$tt`, 12h/24h) and `Date.txt` (`$dd-MM-yy`, `$dd-MM-yyyy`, `$dd MMMM yyyy`, `$dddd dd MMMM yyyy`, localized EN/FR `$date`).
- **Timers & Chronos**: `Countdown.txt` toward a target date/time (`$d`, `$h`, `$m`, `$s`), `ChronoDown.txt` (countdown timer with Start/Pause/Reset), and `ChronoUp.txt` (stopwatch with `$totalminutes`).
- **System Info**: `SystemInfo.txt` (`$CPU_USAGE`, `$RAM_MB_AVAIL`, `$RAM_%_IN_USE`, `$RAM_%_AVAIL`, `$AMOUNT_PROCESSES`, `$UPLOAD`, `$DOWNLOAD`) via native Windows APIs and Linux `/proc`.
- **Textline Changer & Dynamic Files**: sequential or random message rotation (`TextlineChanger.txt`) and periodic HTTP/HTTPS fetching of remote text or image files with custom headers.
- **Discord Rich Presence**: Listening mode (`type: 2`, album shown without repeating the artist), automatic iTunes cover lookup (`- Topic` suffix cleanup and bounded cache), `SET_ACTIVITY` error detection, and automatic reconnection.
- **Twitch (Helix API)**: live viewer counter (`TwitchViewers.txt` via `$viewers`), category search, and stream title/game updates with favorite presets.
- **Settings & Telemetry**: customizable output directory, launch on system startup (`--silent`), and live RAM monitoring for both daemon and GUI.

---

## Building from Source

### Windows (no external DLLs required)
- **Visual Studio 2022/2026 Build Tools** (C++ x64 + Windows 10/11 SDK)
- **CMake 3.20+** and **Ninja**

```bat
windows\build.bat
```
Builds in Release mode, runs unit tests (`osmv_unit_tests`), and copies standalone `OSMV.exe` (~1.94 MB) to the project root.

### Linux
```bash
# Arch Linux (GLFW 3.4+ Wayland + X11)
sudo pacman -S cmake ninja gcc pkgconf dbus curl glfw libgl

# Ubuntu / Debian (add libwayland-dev libwayland-bin libxkbcommon-dev on Ubuntu <= 24.04 for Wayland app_id / GLFW 3.4)
sudo apt install cmake ninja-build g++ pkg-config libdbus-1-dev libcurl4-openssl-dev libglfw3-dev libgl-dev

./linux/build.sh
```
Compiles and copies `./osmv` (daemon) and `./osmv-gui` (GUI) to the project root. On Wayland/Sway, the window exposes `app_id="io.github.streammusicviewer.OSMV"`, and on X11/XWayland `WM_CLASS` (`instance="osmv"`, `class="OSMV"`). A `.desktop` entry and icon are also provided in `linux/` (installed via `cmake --install linux/build`).

---

## OBS Usage

1. **Music Widget**: **Browser** source -> Local file `shared/index.html` (Width `500`, Height `140`).
2. **Dynamic Text**: **Text (GDI+)** / **Text (FreeType 2)** source -> *Read from file* -> select the generated files in `shared/dynamic_text/` (or your custom directory).
3. **Dynamic Images**: **Image** source -> select the image file saved by **Dynamic Files**.

---

## Documentation

Read the full documentation in `docs/` or at [streammusicviewer.github.io/OSMV](https://streammusicviewer.github.io/OSMV/). If you run into issues, check [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

## License
GNU General Public License v3.0 (GPL-3.0) — See [LICENSE](LICENSE) for details.
