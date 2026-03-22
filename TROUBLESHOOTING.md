# Troubleshooting Guide

## Windows Issues

### 1. `compile_rust.bat` fails — `cargo` not found
**Solution**: Ensure you have installed [Rustup](https://rustup.rs/). If you just installed it, restart your terminal or VS Code to refresh the `PATH` environment variable.

### 2. Widget shows "Waiting for music..." but music is playing
- Check if the OSMV window is detecting the song.
- Ensure `shared/index.html` and `current_song.json` are in the **same folder**.
- If using a browser (YouTube, etc.), enable **"Global Media Controls"** in your browser settings.

### 3. Discord Rich Presence is not updating
- Ensure **"Discord Rich Presence  ON"** is toggled in the app.
- Check that your **Application Client ID** is correct.
- Verify that Discord is running and you are logged in.
- If the large image is missing, provide a valid **Fallback large image key** from your Discord Art Assets.

---

## Linux Issues

### 1. D-Bus / MPRIS support
Ensure `libdbus` is installed. Most players (Spotify, VLC, Audacious) support MPRIS2 out of the box.

### 2. No system tray icon appears
Your desktop environment may need an extension:
- **GNOME**: Install [AppIndicator extension](https://extensions.gnome.org/extension/615/appindicator-support/)
- **KDE / XFCE / Cinnamon**: Should work out of the box.

### 3. `./osmv`: Permission denied
```bash
chmod +x osmv
```

### 4. Build fails — missing dependencies
Ensure you have `gcc`, `make`, `pkg-config`, and `libdbus-1-dev` (on Debian/Ubuntu) or `dbus-devel` (on Fedora/Arch).

---

## Support

If your issue persists, open an **Issue** on GitHub and include:
- Your OS and version (Windows 10/11, Ubuntu 24.04, Arch…).
- The music application you are using (Spotify, Apple Music, VLC, Firefox…).
- Output of any error messages from the terminal.
