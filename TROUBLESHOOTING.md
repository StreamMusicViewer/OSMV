# Guide de Dépannage (Troubleshooting)

Ce guide répertorie les problèmes les plus courants lors de la compilation ou de l'utilisation d'OSMV v5.0.0 sous Windows et Linux.

---

## Problèmes de Compilation

### 1. Windows : Compilateur MSVC ou Windows SDK introuvable
- **Description** : `windows\build.bat` signale que Visual Studio C++ Build Tools est absent ou échoue sur les en-têtes `winrt/Windows.Media.Control.h`.
- **Solutions** :
  - Installez **Visual Studio Build Tools** avec la charge de travail *Développement Desktop en C++* (Desktop development with C++) et le **SDK Windows 10/11**.
  - Vérifiez que `cmake` (3.20+) et `ninja` sont disponibles dans le `PATH`.

### 2. Linux : Dépendances système manquantes (`dbus-1`, `libcurl`, `glfw3`, `OpenGL`)
- **Description** : CMake échoue lors de la recherche d'un paquet requis pour `./linux/build.sh`.
- **Solutions** :
  - **Arch Linux** : `sudo pacman -S cmake ninja gcc pkgconf dbus curl glfw-x11 libgl`
  - **Ubuntu / Debian** : `sudo apt install cmake ninja-build g++ pkg-config libdbus-1-dev libcurl4-openssl-dev libglfw3-dev libgl-dev`
  - **Fedora** : `sudo dnf install cmake ninja-build gcc-c++ pkgconf-pkg-config dbus-devel libcurl-devel glfw-devel mesa-libGL-devel`

---

## Problèmes sous Linux

### 1. L'icône de la barre des tâches (Tray Icon) n'apparaît pas
- **Description** : Le daemon `./osmv` tourne mais aucune icône n'est visible dans votre panneau.
- **Solutions** :
  - OSMV implémente directement `org.kde.StatusNotifierItem` et `com.canonical.dbusmenu` via `libdbus-1` (sans GTK).
  - **Hyprland / Sway / Waybar** : Ajoutez le module `"tray"` dans la configuration de votre barre.
  - **GNOME** : Installez et activez l'extension *AppIndicator and KStatusNotifierItem Support*.
  - Vous pouvez toujours rouvrir l'interface à tout moment en lançant `./osmv --gui` ou `./osmv-gui`.

### 2. Le lecteur de musique n'est pas détecté (MPRIS)
- **Description** : L'interface reste sur *En attente de musique...* alors qu'un média est en cours de lecture.
- **Solutions** :
  - Vérifiez que votre lecteur (Spotify, VLC, Audacious, etc.) expose une interface D-Bus `org.mpris.MediaPlayer2.*`.
  - Si vous utilisez plusieurs lecteurs ou un navigateur (Firefox, Chrome), vérifiez que les contrôles média globaux sont activés ou utilisez `playerctld`.

---

## Problèmes sous Windows

### 1. L'application est en arrière-plan et aucune fenêtre n'est visible
- **Description** : OSMV fonctionne en mode daemon headless (ou a été lancé avec `--silent` au démarrage du système).
- **Solutions** :
  - Cliquez ou double-cliquez sur l'icône OSMV dans la zone de notification (System Tray), ou faites un clic droit -> **Afficher la configuration**.
  - Vous pouvez aussi relancer `OSMV.exe` : l'instance existante détectera l'appel via IPC et ouvrira l'interface automatiquement.

### 2. Impossible d'écraser `OSMV.exe` lors d'une recompilation
- **Description** : `windows\build.bat` échoue à la copie de `OSMV.exe` car le daemon tourne encore.
- **Solutions** :
  - Faites un clic droit sur l'icône tray -> **Quitter OSMV** (ou cliquez sur **Quitter OSMV** dans l'interface) avant de relancer `windows\build.bat`.

---

## Discord Rich Presence

### 1. Le statut affiche « Connecté à Discord » mais rien n'apparaît sur le profil
- **Solutions** :
  - Vérifiez que votre statut Discord n'est **pas en Invisible** (en mode Invisible, Discord masque toute Rich Presence).
  - Dans Discord -> **Paramètres utilisateur** -> **Confidentialité des activités**, activez **Partager mes activités détectées avec les autres** ainsi que le partage sur le serveur concerné.
  - Cliquez sur votre avatar en bas à gauche dans Discord pour afficher la carte d'activité complète.

### 2. Le badge affiche « Erreur SET_ACTIVITY » ou « Client ID invalide »
- **Solutions** :
  - Vérifiez que le **Client ID** saisi est bien l'ID numérique de votre application sur `discord.com/developers/applications`.
  - Vérifiez les noms des clés d'images dans **Rich Presence -> Art Assets** (`playing`, `paused`, `stopped` et votre clé de secours).

---

## Twitch Helix & Dynamic Files

### 1. Twitch affiche « Invalid/Expired OAuth Token (401) »
- **Solutions** :
  - Utilisez le bouton **Générer un Token** dans l'onglet Twitch (qui demande le scope `channel:manage:broadcast`) après avoir renseigné votre **Twitch Client ID**, puis collez le nouveau token et sauvegardez.

### 2. Dynamic Files ou les fichiers texte OBS ne se mettent pas à jour
- **Solutions** :
  - Vérifiez le statut affiché sur la règle concernée (code HTTP, URL valide en `http://` ou `https://`, en-têtes `Clé: Valeur`).
  - Assurez-vous qu'OSMV dispose des droits d'écriture sur `shared/dynamic_text/` (évitez d'installer le dossier dans `C:\Program Files`).
