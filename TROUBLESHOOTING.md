# 🔍 Troubleshooting Guide

## ✅ Repository Structure

```
OSMV/
├── src/                 ← C++ source (cross-platform)
├── shared/              ← OBS widget files
│   ├── index.html
│   └── style.css
├── windows/
│   ├── OSMV_logo.ico
│   ├── OSMV.rc
│   └── compile.bat
├── linux/
│   └── compile.sh
└── CMakeLists.txt
```

---

## 🪟 Windows Issues

### 1. Prefer the pre-compiled release
Download from **Releases** → no developer tools needed.

### 2. `compile.bat` fails — Qt 6 not found
**Solution**: Set `QTDIR` before running:
```bat
set QTDIR=C:\Qt\6.7.0\msvc2019_64
windows\compile.bat
```
Download Qt 6 from: https://www.qt.io/download-open-source

### 3. `cmake` is not recognized
**Solution**: Install CMake from https://cmake.org/download/ (**check "Add to PATH"** during install).

### 4. Widget shows "Waiting for music..." but music is playing
- Check the OSMV window is detecting the song.
- Ensure `index.html` and `current_song.json` are in the **same folder**.
- If using a browser (YouTube, etc.), enable **"Global Media Controls"** in your browser.

### 5. Compilation error: `WinRT headers not found`
You need **Windows SDK 10.0.19041+**. Install via Visual Studio Installer → **Desktop development with C++** → check **Windows 10/11 SDK**.

---

## 🐧 Linux Issues

### 1. `playerctl` not found
```bash
sudo pacman -S playerctl        # Arch/Manjaro
sudo apt install playerctl      # Debian/Ubuntu/Mint
sudo dnf install playerctl      # Fedora
```

### 2. Qt 6 not found during build
```bash
sudo pacman -S qt6-base         # Arch/Manjaro
sudo apt install qt6-base-dev   # Ubuntu 24.04+
sudo dnf install qt6-qtbase-devel  # Fedora
```

### 3. No system tray icon appears
Your desktop environment may need an extension:
- **GNOME**: Install [AppIndicator extension](https://extensions.gnome.org/extension/615/appindicator-support/)
- **KDE / XFCE / Cinnamon**: Should work out of the box.

### 4. Widget shows "Nothing playing" but music is **playing**
Your player doesn't expose an MPRIS2 interface. Check which players `playerctl` sees:
```bash
playerctl -l
```
Fixes for common players:
- **Spotify (snap)**: `sudo snap connect spotify:mpris`
- **Firefox**: Enable `media.hardwaremediakeys.enabled` in `about:config`
- **Chrome**: Should work if media controls are enabled

### 5. `./osmv`: Permission denied
```bash
chmod +x osmv
```

### 6. Discord RPC not working
- Discord must be **running** (not just the browser).
- Avoid the Discord **snap** or **flatpak** — they block IPC. Use the official `.tar.gz` from https://discord.com/download.
- Enable Discord RPC in the OSMV settings window.

### 7. `./linux/compile.sh` fails
Check the error output. Common causes:

| Error | Fix |
|---|---|
| `cmake: command not found` | `sudo pacman -S cmake` |
| `Qt6Widgets not found` | `sudo pacman -S qt6-base` |
| `discord_rpc.h not found` | CMake will download it automatically via `FetchContent` — check your internet connection |
| Old cmake version | Update: `sudo pacman -Syu cmake` |

Run the build manually for more details:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --parallel $(nproc)
```

---

## 💡 Support

If your issue persists, open an **Issue** on GitHub and include:
- Your OS and version (Windows 10/11, Ubuntu 24.04, Arch…)
- The music application you are using (Spotify, Apple Music, VLC, Firefox…)
- Output of `playerctl -l` (Linux) or any error messages
- CMake/compiler version: `cmake --version && c++ --version`
