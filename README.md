# OSMV (OBS Stream Music Viewer)

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C)
![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-blueviolet)

> **OSMV v5.0.0** est un outil natif C++20 ultra-léger pour OBS Studio (Now Playing, Discord Rich Presence, et modules Snaz complets : Heure, Date, Countdown, Chrono Down/Up, System Info, Textline Changer, Dynamic Files et Twitch Helix).

---

## Architecture Ultra-Légère en C++20

OSMV sépare le service d'arrière-plan de l'interface graphique pour consommer le strict minimum de RAM en stream :

1. **Daemon d'arrière-plan (~4.7 Mo RAM privée sous Windows / zéro dépendance X11/OpenGL/GTK sous Linux)** :
   - Surveille les lecteurs média (WinRT SMTC sous Windows, D-Bus MPRIS sous Linux), met à jour `shared/current_song.json` et les fichiers texte OBS uniquement lorsqu'un contenu change, gère Discord Rich Presence via IPC natif et affiche l'icône de barre des tâches (Win32 Tray / D-Bus StatusNotifierItem).
2. **Interface graphique Dear ImGui (DX11 sous Windows, GLFW + OpenGL3 sous Linux)** :
   - Rendu événementiel (~0 % CPU au repos), thèmes Sombre/Clair, 5 couleurs d'accentuation, bilingue FR/EN, et libère 100 % de sa RAM dès que vous passez en **Mode Headless** ou fermez la fenêtre `[X]`.

---

## Fonctionnalités

- **Now Playing** : widget OBS `shared/index.html` (pochette, couleur dynamique, visualiseur audio) + exports texte style Snaz (`TrackArtist.txt`, `TrackSong.txt`, `TrackAlbum.txt`, `PlayingNow.txt`, `track-info.json`).
- **Temps & Date** : `Time.txt` (`$h`, `$m`, `$s`, `$tt`) et `Date.txt` (`$dd-MM-yy`, `$dd-MM-yyyy`, `$dd MMMM yyyy`, `$dddd dd MMMM yyyy`, `$date`).
- **Minuteurs & Chronos** : `Countdown.txt` (date cible), `ChronoDown.txt` (minuteur avec Start/Pause/Reset), `ChronoUp.txt` (chronomètre avec `$totalminutes`).
- **System Info** : `SystemInfo.txt` (`$CPU_USAGE`, `$RAM_MB_AVAIL`, `$RAM_%_IN_USE`, `$RAM_%_AVAIL`, `$AMOUNT_PROCESSES`, `$UPLOAD`, `$DOWNLOAD`) via APIs natives / `/proc`.
- **Textline Changer & Dynamic Files** : rotation de messages dans un `.txt` et récupération HTTP/HTTPS périodique de fichiers texte ou image.
- **Discord Rich Presence** : pochettes automatiques via iTunes Search API + badges de statut et mode idle personnalisable.
- **Twitch (API Helix)** : compteur de viewers (`TwitchViewers.txt`) et mise à jour titre / catégorie avec jeux favoris.

---

## Compilation

### Windows (aucune DLL externe requise)
- **Visual Studio 2022/2026 Build Tools** (C++ x64 + SDK Windows 10/11)
- **CMake 3.20+** et **Ninja**

```bat
windows\build.bat
```
Produit `OSMV.exe` à la racine du projet (~2 Mo, autonome).

### Linux
```bash
# Arch Linux
sudo pacman -S cmake ninja gcc pkgconf dbus curl glfw-x11 libgl

# Ubuntu / Debian
sudo apt install cmake ninja-build g++ pkg-config libdbus-1-dev libcurl4-openssl-dev libglfw3-dev libgl-dev

./linux/build.sh
```
Produit `./osmv` (daemon sans dépendance graphique) et `./osmv-gui` à la racine du projet.

---

## Utilisation OBS

1. **Widget Musique** : Source **Navigateur** ➔ Fichier local `shared/index.html` (Largeur `500`, Hauteur `140`).
2. **Textes Dynamiques** : Source **Texte (GDI+)** / **Texte (FreeType 2)** ➔ *Lire depuis un fichier* ➔ sélectionnez les fichiers générés dans `shared/dynamic_text/`.

---

## Licence
Licence MIT — Libre d'utilisation personnelle et commerciale.
