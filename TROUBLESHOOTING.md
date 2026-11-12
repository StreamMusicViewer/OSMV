# Troubleshooting Guide

This guide covers the most common issues when compiling or running OSMV v5.0.0 on Windows and Linux.

---

## Build Issues

### 1. Windows: MSVC compiler or Windows SDK not found
- **Description**: `windows\build.bat` reports missing Visual Studio C++ Build Tools or fails on `winrt/Windows.Media.Control.h` headers.
- **Solutions**:
  - Install **Visual Studio Build Tools** with the **Desktop development with C++** workload and the **Windows 10/11 SDK**.
  - Verify that `cmake` (3.20+) and `ninja` are available in your `PATH`.

### 2. Linux: Missing system dependencies (`dbus-1`, `libcurl`, `glfw3`, `OpenGL`)
- **Description**: CMake fails while locating a required package for `./linux/build.sh`.
- **Solutions**:
  - **Arch Linux**: `sudo pacman -S cmake ninja gcc pkgconf dbus curl glfw libgl`
  - **Ubuntu / Debian**: `sudo apt install cmake ninja-build g++ pkg-config libdbus-1-dev libcurl4-openssl-dev libglfw3-dev libgl-dev` (on Ubuntu 24.04 or Debian 12 where `libglfw3-dev` is 3.3, also install `libwayland-dev libwayland-bin libxkbcommon-dev` so CMake can fetch GLFW 3.4 with native Wayland `app_id` support)
  - **Fedora**: `sudo dnf install cmake ninja-build gcc-c++ pkgconf-pkg-config dbus-devel libcurl-devel glfw-devel mesa-libGL-devel`

### 3. Build errors after pulling header changes on Windows
- **Description**: Localized MSVC compilers can omit header dependency tracking under Ninja.
- **Solution**: `windows\build.bat` automatically runs `cmake --build windows\build --clean-first` so all translation units recompile consistently.

---

## OBS Browser Overlay Studio

### 1. A `?preset=...` Scene URL Shows "Nothing currently playing"
- **Description**: In OBS Browser Source properties, you pasted `file:///.../shared/index.html?preset=MyScene` with **Local file** unchecked, and the preset style loads while track info stays idle.
- **Solutions**:
  - Make sure you are running an updated `OSMV` daemon so both `shared/current_song.js` and `shared/widget_config.js` are generated alongside the `.json` files (Chromium/OBS blocks `fetch()` on `file://` URLs via CORS, so `shared/widget.js` loads `.js` script fallbacks automatically).
  - Verify the preset name in `?preset=<Name>` matches a built-in preset (`classic`, `glass`, `neon`, `compact`, `vinyl`, `minimal`, `banner`) or one of your **Saved Presets** (case-insensitive).

### 2. Custom CSS or Style Edits Do Not Update on a Scene URL
- **Solutions**:
  - If that scene uses `?preset=<Name>`, click **Update** next to your saved preset in **Now Playing -> Presets & Style -> Saved Presets (Multi-Scene OBS)** after modifying settings or Custom CSS.
  - Enable **Enable custom CSS** in the **Custom CSS** tab before expecting rules to apply, and click **Refresh cache of current page** in OBS Browser Source properties.

---

## Linux Issues

### 1. Workspace / Window Rules (Sway, Hyprland, i3)
- **Identifiers exposed by `osmv-gui`**:
  - **Wayland (`xdg_shell`, GLFW 3.4+)**: `app_id="io.github.streammusicviewer.OSMV"`
  - **X11 / XWayland (`WM_CLASS`)**: `instance="osmv"`, `class="OSMV"`
- **Sway example**:
  ```sway
  assign [app_id="io.github.streammusicviewer.OSMV"] workspace number 9
  ```
- If `swaymsg -t get_tree` does not show `app_id`, verify OSMV was compiled against **GLFW 3.4+** rather than GLFW 3.3 (see the Linux dependencies section above).

### 2. System Tray Icon Is Invisible
- **Description**: The `./osmv` daemon is running, but no icon appears in your bar/panel.
- **Solutions**:
  - OSMV implements `org.kde.StatusNotifierItem` and `com.canonical.dbusmenu` directly over `libdbus-1` (no GTK dependency required).
  - **Hyprland / Sway / Waybar**: Add the `"tray"` module to your Waybar configuration.
  - **GNOME**: Install and enable the *AppIndicator and KStatusNotifierItem Support* extension.
  - You can always reopen the GUI manually by running `./osmv --gui` or `./osmv-gui`.

### 3. Music Player Is Not Detected (MPRIS)
- **Description**: The interface stays on *Waiting for music...* while a track is playing.
- **Solutions**:
  - Make sure your player (Spotify, VLC, Audacious, etc.) exposes a D-Bus `org.mpris.MediaPlayer2.*` interface.
  - If using multiple players or a web browser (Firefox, Chrome), ensure global media controls are enabled or use `playerctld`.

---

## Windows Issues

### 1. The Application Is Running in the Background With No Visible Window
- **Description**: OSMV is running in headless daemon mode (or was started with `--silent` at login).
- **Solutions**:
  - Click or double-click the OSMV icon in the Windows notification area (System Tray), or right-click -> **Show Configuration**.
  - You can also launch `OSMV.exe` again: the running instance detects it over IPC and opens the GUI automatically.

### 2. Cannot Overwrite `OSMV.exe` When Recompiling
- **Description**: `windows\build.bat` fails to copy `OSMV.exe` because the daemon is still running.
- **Solutions**:
  - Right-click the tray icon -> **Quit OSMV** (or click **Quit OSMV** in the GUI) before running `windows\build.bat`.

---

## Discord Rich Presence

### 1. Status Shows "Connected to Discord" but Nothing Appears on Your Profile
- **Solutions**:
  - Make sure your Discord status is **not Invisible** (Discord hides all Rich Presence while Invisible).
  - In Discord -> **User Settings** -> **Activity Privacy**, enable sharing detected activity and per-server activity sharing.
  - Click your avatar at the bottom-left of Discord to inspect the full activity card.

### 2. Status Badge Shows "SET_ACTIVITY error (check keys/assets)" or "Invalid Client ID"
- **Solutions**:
  - Verify that **Application Client ID** matches the numeric Application ID from `discord.com/developers/applications`.
  - Check your asset key names under **Rich Presence -> Art Assets** (`playing`, `paused`, `stopped`, and your fallback image key).

---

## Twitch Helix & Dynamic Files

### 1. Twitch Shows "Invalid/Expired OAuth Token (401)"
- **Solutions**:
  - Enter your **Twitch Client ID** and click **Get OAuth Token** in the Twitch tab (requests the `channel:manage:broadcast` scope). OSMV captures the localhost redirect automatically; if port 80 is busy or restricted, paste the full redirect URL into the Token field and save.

### 2. Dynamic Files or OBS Text Files Are Not Updating
- **Solutions**:
  - Check the status badge on the rule (HTTP status code, valid `http://` or `https://` URL, `Key: Value` headers).
  - Ensure OSMV has write permission to `shared/dynamic_text/` (avoid placing the portable folder inside `C:\Program Files`).
