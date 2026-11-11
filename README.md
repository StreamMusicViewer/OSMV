# OSMV (OBS Stream Music Viewer)

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C)
![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-blueviolet)

> **OSMV v5.0.0** est un outil natif C++20 ultra-léger pour OBS Studio (Now Playing, Discord Rich Presence, et modules inspirés de Snaz : Heure, Date, Countdown, Chrono Down/Up, System Info, Textline Changer, Dynamic Files et Twitch Helix).

---

## Architecture Ultra-Légère en C++20

OSMV sépare le service d'arrière-plan de l'interface graphique afin de limiter au maximum l'empreinte mémoire et CPU pendant un stream :

1. **Daemon d'arrière-plan (~4,5 à 6 Mo de RAM privée sous Windows / zéro dépendance X11, Wayland, OpenGL ou GTK sous Linux)** :
   - Surveille les lecteurs média via WinRT SMTC (Windows) ou D-Bus MPRIS (Linux), avec mise en cache de la pochette uniquement lors d'un changement de piste.
   - Met à jour `shared/current_song.json` et les fichiers texte OBS uniquement lorsque leur contenu change (écritures atomiques).
   - Exécute Discord Rich Presence (IPC natif), les requêtes HTTP (Dynamic Files / Twitch sur un thread réseau dédié) et l'icône de barre des tâches (`Shell_NotifyIconW` sous Windows, D-Bus `StatusNotifierItem` + `dbusmenu` sous Linux).
   - Expose un serveur IPC local (`\\.\pipe\osmv-ipc-v3` sous Windows, `$XDG_RUNTIME_DIR/osmv-ipc-v3.sock` sous Linux) pour communiquer avec l'interface.
2. **Interface graphique Dear ImGui (DirectX 11 sous Windows, GLFW + OpenGL 3 sous Linux)** :
   - Rendu événementiel (~0 % CPU au repos), thèmes Sombre/Clair, 5 couleurs d'accentuation, bilingue FR/EN, puces d'insertion de codes et copie rapide des chemins OBS.
   - Sous Windows, les DLL graphiques (`d3d11.dll`, `dxgi.dll`, `dwmapi.dll`) sont chargées en `/DELAYLOAD` uniquement en mode GUI.
   - Fermer la fenêtre `[X]` ou cliquer sur **Mode Headless** coupe le processus GUI et libère toute sa RAM (~51 à 58 Mo sous Windows) tout en laissant le daemon actif.

---

## Fonctionnalités

- **Now Playing** : widget OBS `shared/index.html` (pochette, couleur dynamique, visualiseur audio) + exports texte/JSON façon Snaz (`TrackArtist.txt`, `TrackSong.txt`, `TrackAlbum.txt`, `PlayingNow.txt` avec `$artist`, `$song`, `$album`, `*_upper`, et `track-info.json`).
- **Temps & Date** : `Time.txt` (`$h`, `$m`, `$s`, `$tt`, 12h/24h) et `Date.txt` (`$dd-MM-yy`, `$dd-MM-yyyy`, `$dd MMMM yyyy`, `$dddd dd MMMM yyyy`, `$date` localisé FR/EN).
- **Minuteurs & Chronos** : `Countdown.txt` vers une date/heure cible (`$d`, `$h`, `$m`, `$s`), `ChronoDown.txt` (minuteur avec Start/Pause/Reset) et `ChronoUp.txt` (chronomètre avec `$totalminutes`).
- **System Info** : `SystemInfo.txt` (`$CPU_USAGE`, `$RAM_MB_AVAIL`, `$RAM_%_IN_USE`, `$RAM_%_AVAIL`, `$AMOUNT_PROCESSES`, `$UPLOAD`, `$DOWNLOAD`) via les APIs natives Windows et `/proc` sous Linux.
- **Textline Changer & Dynamic Files** : rotation séquentielle ou aléatoire de messages (`TextlineChanger.txt`) et récupération HTTP/HTTPS périodique de fichiers texte ou image avec en-têtes personnalisés.
- **Discord Rich Presence** : mode « Écoute » (`type: 2`, affichage de l'album sans répétition d'artiste), recherche automatique de pochettes iTunes (avec nettoyage du suffixe `- Topic` et cache borné), détection d'erreurs `SET_ACTIVITY` et reconnexion automatique.
- **Twitch (API Helix)** : compteur de viewers (`TwitchViewers.txt` via `$viewers`), recherche de catégories et mise à jour titre/jeu avec jeux favoris.
- **Paramètres & Télémétrie** : dossier de sortie personnalisable, lancement automatique au démarrage (`--silent`), et mesure RAM en direct du daemon et de l'interface.

---

## Compilation

### Windows (aucune DLL externe requise)
- **Visual Studio 2022/2026 Build Tools** (C++ x64 + SDK Windows 10/11)
- **CMake 3.20+** et **Ninja**

```bat
windows\build.bat
```
Compile en Release, exécute les tests unitaires (`osmv_unit_tests`) et copie `OSMV.exe` (~1,94 Mo, autonome) à la racine du projet.

### Linux
```bash
# Arch Linux
sudo pacman -S cmake ninja gcc pkgconf dbus curl glfw-x11 libgl

# Ubuntu / Debian
sudo apt install cmake ninja-build g++ pkg-config libdbus-1-dev libcurl4-openssl-dev libglfw3-dev libgl-dev

./linux/build.sh
```
Compile et copie `./osmv` (daemon) et `./osmv-gui` (interface) à la racine du projet.

---

## Utilisation OBS

1. **Widget Musique** : Source **Navigateur** -> Fichier local `shared/index.html` (Largeur `500`, Hauteur `140`).
2. **Textes Dynamiques** : Source **Texte (GDI+)** / **Texte (FreeType 2)** -> *Lire depuis un fichier* -> sélectionnez les fichiers générés dans `shared/dynamic_text/` (ou votre dossier personnalisé).
3. **Images Dynamiques** : Source **Image** -> sélectionnez le fichier image généré par **Dynamic Files**.

---

## Documentation

Retrouvez la documentation complète dans `docs/` et sur [streammusicviewer.github.io/OSMV](https://streammusicviewer.github.io/OSMV/). En cas de problème, consultez [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

## Licence
Licence MIT — Libre d'utilisation personnelle et commerciale.
