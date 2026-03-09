# 🎵 Now Playing Widget for OBS

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![.NET](https://img.shields.io/badge/.NET-8.0-purple)
![Python](https://img.shields.io/badge/Python-3.10%2B-yellow)

A real-time "Now Playing" widget for OBS that displays currently playing music with album artwork and animated transitions. Available on both **Windows** and **Linux**!

## ✨ Features

- 🎵 **Real-time updates** — Detects currently playing music every second
- 🖼️ **Album artwork** — Displays full-resolution album covers
- 🎨 **Dynamic color** — Widget background matches the album cover palette
- 🎚️ **Audio visualizer** — Animated bars in OBS (beta)
- 🎮 **Discord Rich Presence** — Shows what you're listening to on Discord
- 🔄 **Background operation** — Minimize to system tray
- 🎯 **Multi-app support** — Spotify, Apple Music, Firefox, Chrome, VLC, and more

## 📁 Repository Structure

```
OSMV/
├── shared/          ← OBS browser source widget (Windows & Linux)
│   ├── index.html
│   └── style.css
├── windows/         ← Windows app (C# / WPF / .NET 8)
│   ├── *.cs / *.xaml
│   ├── OBS-StreamMusicViewer.csproj
│   └── compile.bat
└── linux/           ← Linux app (Python 3 / MPRIS2 / pystray)
    ├── osmv.py
    ├── discord_rpc_service.py
    ├── requirements.txt
    └── compile.sh
```

## 🏗️ How It Works

```
Music Player (Spotify, Apple Music, VLC…)
    ↓
Windows: GlobalSystemMediaTransportControlsSessionManager (WinRT)
Linux:   playerctl + MPRIS2 D-Bus
    ↓
OSMV application (writes current_song.json every ~1s)
    ↓
shared/index.html  (OBS Browser Source, polls JSON)
    ↓
OBS overlay
```

---

## 🚀 Quick Start

### Windows

1. Go to the **[Releases](../../releases)** page and download the latest `.zip`.
2. Extract and place `OBS-StreamMusicViewer.exe`, `index.html`, and `style.css` in a folder.
3. Double-click `OBS-StreamMusicViewer.exe`.
4. Configure OBS (see below).

### Linux

1. **Install system dependencies:**
   ```bash
   sudo apt install playerctl python3 python3-pip python3-venv
   ```
2. Go to the **[Releases](../../releases)** page and download the latest Linux `.tar.gz`.
3. Extract and place `osmv`, `index.html`, and `style.css` in a folder.
4. Run `./osmv` — an icon will appear in your system tray.
5. Configure OBS (see below).

---

## 📺 Configure OBS

1. In OBS, add a new **Browser** source
2. ☑️ Check **"Local file"**
3. 📁 Browse and select `index.html` from the folder containing the app
4. Set dimensions: **Width: 500**, **Height: 140**
5. Click OK

*As long as the application is running, your OBS widget updates automatically.*

---

## 🔧 Compiling from Source

### Windows

**Requirements:** Windows 10/11, .NET 8.0 SDK

```bat
cd windows
compile.bat
```

Copy `OBS-StreamMusicViewer.exe` along with `../shared/index.html` and `../shared/style.css` into your deployment folder.

### Linux

**Requirements:** Python 3.10+, `playerctl`

```bash
sudo apt install playerctl python3 python3-pip python3-venv
cd linux
./compile.sh
```

The compiled binary will be in `linux/dist/osmv`. Copy it together with `../shared/index.html` and `../shared/style.css` into your deployment folder.

**Or run directly without compiling:**
```bash
cd linux
pip install -r requirements.txt
python3 osmv.py
```

---

## 🎨 Customization

Edit `shared/style.css` to customize the OBS widget appearance:
- Colors and transparency
- Album artwork size
- Widget position
- Animation effects

---

## 🐛 Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

---

## 🤝 Contributing

Contributions are welcome! Feel free to open issues or submit pull requests.

## 📄 License
MIT License — free for personal and commercial use.

