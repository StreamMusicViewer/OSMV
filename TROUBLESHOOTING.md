# 🔍 Troubleshooting Guide

## ✅ Checking Source Files

If you cloned or downloaded the source code to compile it yourself, make sure you have these main files:

```
OBS-StreamMusicViewer/
├── .gitignore
├── App.xaml                  ← Base interface
├── App.xaml.cs
├── DiscordRpcService.cs      ← Discord Rich Presence logic
├── MainWindow.xaml           ← Window design
├── MainWindow.xaml.cs        ← Music retrieval & core logic
├── OBS-StreamMusicViewer.csproj ← CRITICAL - project file
├── OSMV_logo.ico             ← Application icon
├── README.md
├── TROUBLESHOOTING.md
├── compile.bat               ← Compilation script
├── current_song.json         ← Generated data (after run)
├── index.html                ← OBS display
├── settings.json             ← Saved user preferences
└── style.css                 ← OBS styles
```

## 🐛 Common Issues & Solutions

### 1. The easiest way (No need to compile!)
If you encounter compilation errors, skip the command line and simply download the **Release**.
1. Go to the **Releases** tab on GitHub.
2. Download the `OBS-StreamMusicViewer.exe` executable or the ZIP file containing the release.
3. Run the `.exe` file. No development tools or command line required!

### 2. The "compile.bat" script shows a namespace / missing project error
**Cause**: The `.csproj` file is not found by the `dotnet` command, or the clone went wrong.
**Solution**: Make sure you are in the correct folder. You can also download the source code ZIP (`Code → Download ZIP`) from GitHub to ensure you have all files intact.

### 3. "dotnet is not recognized as a command"
**Cause**: The .NET SDK is not installed on your computer.
**Solution**:
1. Install it from https://dotnet.microsoft.com/download/dotnet
2. **Restart** your terminal or PC so the environment variable is picked up, then run `compile.bat` again.

### 4. The OBS widget shows "Waiting for music..." but music is playing
**Cause**: The built-in web browser (OBS) or the program (`OBS-StreamMusicViewer.exe`) has a permissions issue, or the music application is not broadcasting the info to Windows.
**Solution**:
- Check that the `OBS-StreamMusicViewer.exe` window is correctly detecting the music. If it is, the issue is on the OBS side.
- Make sure the `index.html` file opened in OBS is located **in the same folder** as `current_song.json`.
- If a browser source (e.g. Chrome/YouTube) is playing the music, check that "Global Media Controls" are not disabled in your browser settings.

## 💡 Support

If your issue persists even with the pre-compiled release, open an **Issue** on GitHub and include:
- The observed behavior and the music application you are using (Spotify, Apple Music, Browser…)
- Your Windows version (10 or 11)
