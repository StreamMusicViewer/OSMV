# 🔍 Troubleshooting Guide

## ✅ Checking Source Files

Make sure you have the expected structure after cloning:

```
OSMV/
├── shared/
│   ├── index.html
│   └── style.css
├── windows/
│   ├── App.xaml / App.xaml.cs
│   ├── DiscordRpcService.cs
│   ├── MainWindow.xaml / MainWindow.xaml.cs
│   ├── OBS-StreamMusicViewer.csproj
│   ├── OSMV_logo.ico
│   └── compile.bat
└── linux/
    ├── osmv.py
    ├── discord_rpc_service.py
    ├── requirements.txt
    └── compile.sh
```

---

## 🪟 Windows Issues

### 1. Prefer the pre-compiled release
If you encounter compilation errors, simply download the **Release** from the GitHub Releases tab — no developer tools required.

### 2. `compile.bat` shows a namespace / missing project error
**Cause**: The `.csproj` file is not found.  
**Solution**: Run `compile.bat` from inside the `windows/` folder, or use `cd windows` first.

### 3. `dotnet` is not recognized
**Cause**: .NET SDK not installed.  
**Solution**: Install from https://dotnet.microsoft.com/download/dotnet then **restart** your terminal.

### 4. Widget shows "Waiting for music..." but music is playing
**Cause**: Permissions issue or the music app doesn't broadcast to Windows Media Controls.  
**Solutions**:
- Check that the OSMV window itself is detecting the song.
- Make sure `index.html` is opened in OBS from the **same folder** as `current_song.json`.
- If using a browser (YouTube, etc.), make sure "Global Media Controls" are enabled in your browser.

---

## 🐧 Linux Issues

### 1. `playerctl not found`
**Cause**: `playerctl` is not installed.  
**Solution**:
```bash
sudo pacman -S playerctl        # Arch/Manjaro
sudo apt install playerctl      # Debian/Ubuntu/Mint
sudo dnf install playerctl      # Fedora
```

### 2. `ImportError: libtk8.6.so` / tkinker not found
**Cause**: The `tk` library is not installed (required by the settings window).  
**Solution**:
```bash
sudo pacman -S tk               # Arch/Manjaro
sudo apt install python3-tk     # Debian/Ubuntu/Mint
sudo dnf install python3-tkinter # Fedora
```

### 3. No system tray icon appears
**Cause**: Missing `pystray` or `Pillow` Python packages, or your desktop environment doesn't support tray icons.  
**Solution**:
```bash
pip install pystray Pillow
```
For GNOME users, install the [AppIndicator extension](https://extensions.gnome.org/extension/615/appindicator-support/) to enable system tray icons.

### 3. Widget shows "Nothing currently playing" but music is playing
**Cause**: Your media player doesn't expose an MPRIS2 interface, or `playerctl` can't see it.  
**Solution**: Check which players `playerctl` detects:
```bash
playerctl -l
```
If your player isn't listed:
- **Spotify (snap)**: Try `sudo snap connect spotify:mpris`
- **Firefox**: Enable `media.hardwaremediakeys.enabled` in `about:config`
- **Chrome/Chromium**: Should work out of the box with recent versions

### 4. Permission denied when running `./osmv`
```bash
chmod +x osmv
```

### 5. Discord RPC not working on Linux
**Cause**: Discord must be running, and `pypresence` must be installed.  
**Solution**:
```bash
pip install pypresence
```
Also ensure the Discord app is running (not just the browser). The Discord snap/flatpak sometimes blocks IPC — prefer the official `.tar.gz` from the Discord website.

### 6. `./compile.sh` fails with PyInstaller errors
**Solution**:
```bash
pip install --upgrade pyinstaller
cd linux && ./compile.sh
```
If the issue persists, run the app directly instead:
```bash
python3 osmv.py
```

---

## 💡 Support

If your issue persists, open an **Issue** on GitHub and include:
- The observed behavior
- Your OS and version (Windows 10/11, Ubuntu 22.04, Arch…)
- The music application you are using (Spotify, Apple Music, VLC, Firefox…)
- Output of `playerctl -l` (Linux) or any error messages shown

