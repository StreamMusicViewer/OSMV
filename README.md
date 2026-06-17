# OSMV (OBS Stream Music Viewer) 🎵

![Status](https://img.shields.io/badge/status-working-success)
![Platform Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Platform Linux](https://img.shields.io/badge/platform-Linux-orange)
![Rust](https://img.shields.io/badge/Language-Rust-brown)
![Qt6](https://img.shields.io/badge/GUI-Qt%206%20QML-blueviolet)


> **OSMV** est un widget "Now Playing" en temps réel et ultra-léger pour OBS qui affiche votre musique avec pochette d'album et transitions animées, couplé à une intégration Discord Rich Presence hautement personnalisable.

---

## 🚀 Nouvelle Architecture Ultra-Légère ⚡

Afin de garantir un impact minimal sur les performances en stream, OSMV est maintenant divisé en deux rôles distincts au sein d'un seul exécutable :

1. **Le Daemon d'Arrière-plan (~15 Mo RAM)** : 
   - Rôle principal. Il tourne en arrière-plan, gère l'écoute des lecteurs média (via WinRT sur Windows et MPRIS sur Linux), met à jour le fichier `current_song.json` en temps réel, met à jour Discord Rich Presence et gère l'icône de la barre des tâches (System Tray).
2. **L'Interface de Configuration QML (~378 Mo RAM)** :
   - Lancé via `osmv --gui` (ou automatiquement au premier démarrage). C'est une interface moderne en **Qt 6 QML** avec des effets de glassmorphism.
   - Elle vous permet de configurer l'application. Dès que vous la fermez (via le bouton **Passer en Headless** dans l'application ou l'icône de fermeture standard), **le processus d'interface se coupe complètement, libérant instantanément les 378 Mo de RAM**. Le Daemon léger continue de tourner de façon transparente.
   - Pour réafficher l'interface, faites un clic droit sur l'icône de la barre des tâches ➔ **Afficher Configuration**.

---

## 📖 Documentation Complète (GitHub Pages)

Retrouvez toute la documentation interactive sur [streammusicviewer.github.io/OSMV](https://streammusicviewer.github.io/OSMV/) :

- ⚙️ **[Installation & Dépendances](https://streammusicviewer.github.io/OSMV/docs/installation/)** — Installer Qt6, Clang, D-Bus, GTK3 et compiler.
- 📐 **[Architecture & Performance](https://streammusicviewer.github.io/OSMV/docs/architecture/)** — Fonctionnement interne du daemon et du GUI.
- 🛠️ **[Configuration OBS & Discord](https://streammusicviewer.github.io/OSMV/docs/obs/)** — Configurer les widgets OBS et l'intégration Discord RPC.
- ❓ **[Foire Aux Questions (FAQ)](https://streammusicviewer.github.io/OSMV/docs/faq/)** — Résolution des problèmes d'icônes, d'autostart, etc.

---

## 🛠️ Dépendances de Développement en Bref

Pour coder sur le projet ou le compiler vous-même, vous avez besoin des dépendances suivantes :

### Linux (ex: Arch Linux)
```bash
sudo pacman -S rust clang qt6-declarative qt6-languageserver gtk3 dbus pkgconf
```

### Windows
- **Rustup** (compilateur stable-x86_64-pc-windows-msvc)
- **Visual Studio Build Tools** (avec le SDK C++)
- **Qt 6.6+** (configuré dans le PATH)

Pour lancer le build :
- **Linux** : `./linux/build_rust.sh`
- **Windows** : `windows\compile_rust.bat`

---

## 📺 Utilisation OBS

1. Ajoutez une source **Navigateur** (Browser Source) dans OBS.
2. Cochez **Fichier local** et sélectionnez `shared/index.html`.
3. Définissez la taille sur **Largeur : 500**, **Hauteur : 140**.

---

## 📄 Licence
Licence MIT — Libre d'utilisation personnelle et commerciale.

## 👤 Auteur
[Ulyxx3](https://github.com/Ulyxx3)
