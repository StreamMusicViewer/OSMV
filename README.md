# 🎵 Now Playing Widget for OBS

![Status](https://img.shields.io/badge/status-working-success)
![Platform](https://img.shields.io/badge/platform-Windows%2011-blue)
![.NET](https://img.shields.io/badge/.NET-8.0-purple)

A real-time "Now Playing" widget for OBS that displays currently playing music from Apple Music (and other media apps) with album artwork. It runs entirely as a lightweight standalone Windows application!

## ✨ Features

- 🎵 **Real-time updates** - Automatically detects currently playing music
- 🖼️ **Album artwork** - Displays full-resolution album covers
- 🎨 **Modern UI** - Sleek, dark-themed, borderless window
- 🔄 **Background operation** - Works even when the music app isn't the active window
- 🎯 **Multi-app support** - Compatible with Apple Music, Spotify, Chrome, Edge, and more
- 🪟 **Native Windows API** - Uses `GlobalSystemMediaTransportControlsSessionManager` for reliable detection
- ⚡ **Zero Dependencies** - Self-contained single executable, no Python required!

## 📸 Preview

The widget application displays:
- Song title & Artist name
- High quality album artwork
- Playback status

And behind the scenes, it continually exports to a `current_song.json` file which powers the OBS browser source with a clean, transparent, glassmorphism design.

## 🚀 Quick Start (Easiest Way)

You do not need to install any programming languages or tools if you just want to use the application.

1. Go to the **[Releases](../../releases)** page on this GitHub repository.
2. Download the latest `OBS-StreamMusicViewer.exe` (or the provided `.zip` file).
3. Place `.exe`, `index.html`, and `style.css` in a single folder.
4. Double click `OBS-StreamMusicViewer.exe` to start the tracker window.
5. Setup OBS (see below).

---

## 🔧 Compiling from Source

If you want to modify the code or compile it yourself:

### Requirements
- **Windows 10/11**
- **.NET 8.0 SDK** (Download from: https://dotnet.microsoft.com/download/dotnet)

### Compilation Steps
1. Clone the repository: `git clone https://github.com/Ulyxx3/OBS-StreamMusicViewer.git`
2. Run the provided script: `compile.bat`
3. The script will generate the standalone `OBS-StreamMusicViewer.exe` executable.

## 📺 Configure OBS

1. In OBS, add a new **Browser** source
2. ☑️ Check "Local file"
3. 📁 Browse and select `index.html` from the extracted folder
4. Set dimensions: **Width: 500**, **Height: 140**
5. Click OK

*As long as `OBS-StreamMusicViewer.exe` is running, your OBS widget will update instantly.*

## 🎨 Customization

Edit `style.css` to customize the OBS visual aspect:
- Colors and transparency
- Album artwork size
- Widget position
- Animation effects

## 🏗️ How It Works

```
Apple Music/Spotify
    ↓
Windows Media Control API
    ↓
OBS-StreamMusicViewer.exe (C# WPF App)
    ↓
current_song.json
    ↓
index.html (Frontend with polling)
    ↓
OBS Browser Source
```

The C# application gracefully accesses Windows Runtime APIs to retrieve media information and continuously exports it as JSON. The HTML interface polls this JSON file to beautifully animate changes in OBS.

## 🐛 Troubleshooting

See the [TROUBLESHOOTING.md](TROUBLESHOOTING.md) guide.

## 🤩 Future
In the future, i want to add :
- an audio visualiser
- a bar for where we are on the music 
- a better support for sites (like newgrounds) where there is no cover or artist support (maybe get rid of the Windows Media Control limits)
- a Linux version
- a whole interface on the app for controlling what is in the final output (show/hide the bar, show/hide the visualizer)

## 🤝 Contributing
Contributions are welcome! Feel free to open issues or submit pull requests.

## 📄 License
MIT License - feel free to use this project for personal or commercial purposes.
