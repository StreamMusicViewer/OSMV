# Guide de Dépannage (Troubleshooting) 🛠️

Ce guide répertorie les problèmes les plus courants lors du développement, de la compilation ou de l'utilisation d'OSMV. Pour des explications exhaustives, consultez la [Section Troubleshooting du site de doc](docs/troubleshooting.md).

---

## 🏗️ Problèmes de Compilation

### 1. Erreur : `Clang / LLVM not found` or `LIBCLANG_PATH`
- **Description** : CXX-Qt a besoin de `clang` pour analyser les fichiers d'en-tête C++ et Rust.
- **Solutions** :
  - **Linux (Arch)** : `sudo pacman -S clang`
  - **Linux (Ubuntu/Debian)** : `sudo apt install clang libclang-dev`
  - **Windows** : Installez LLVM via [chocolatey](https://chocolatey.org/) (`choco install llvm`) ou téléchargez l'installeur officiel et ajoutez le répertoire `bin` de LLVM à votre variable d'environnement `PATH`.

### 2. CMake / Qt 6 introuvable
- **Description** : `cxx-qt-build` ne trouve pas l'installation de Qt 6.
- **Solutions** :
  - Assurez-vous d'avoir installé le package de développement Qt6 (ex: `qt6-base`, `qt6-declarative`).
  - Définissez la variable d'environnement `QT_VERSION=6`.
  - Si Qt est installé dans un dossier personnalisé, définissez la variable d'environnement `CMAKE_PREFIX_PATH` pointant vers le dossier `lib/cmake` de votre installation Qt.

---

## 🐧 Problèmes sous Linux (Arch, Hyprland, Ubuntu...)

### 1. L'icône de la barre des tâches (Tray Icon) n'apparaît pas
- **Description** : Le daemon tourne mais aucune icône n'est visible dans le panel.
- **Solutions** :
  - **Hyprland / Waybar** : Assurez-vous d'avoir ajouté le module `tray` dans la configuration de votre Waybar (`config.jsonc`).
  - **GNOME** : GNOME ne supporte pas les icônes de notification par défaut. Vous devez installer l'extension [AppIndicator and KStatusNotifierItem Support](https://extensions.gnome.org/extension/615/appindicator-support/).
  - **Dépendances manquantes** : Vérifiez que `libappindicator-gtk3` ou `libayatana-appindicator3` est installé.

### 2. Le lecteur de musique n'est pas détecté (MPRIS)
- **Description** : L'application affiche "Waiting for music..." alors qu'une musique tourne.
- **Solutions** :
  - Assurez-vous que votre lecteur (Spotify, Audacious, VLC, Firefox...) est bien compatible MPRIS et actif.
  - Pour les navigateurs (Chrome, Firefox), vérifiez que les **Contrôles de médias globaux (Global Media Controls)** sont activés dans les flags de votre navigateur.

---

## 🪟 Problèmes sous Windows

### 1. L'application se lance mais aucune fenêtre ne s'affiche
- **Description** : L'application a pu démarrer en mode "Daemon" en tâche de fond.
- **Solutions** :
  - Double-cliquez sur l'icône OSMV dans la zone de notification (System Tray) ou faites un clic droit puis **Afficher Configuration**.
  - Si le daemon s'est bloqué, tuez le processus `osmv.exe` dans le Gestionnaire des tâches et relancez-le.

### 2. Discord RPC ne se met pas à jour
- **Solutions** :
  - Assurez-vous que l'option Discord RPC est sur **ON** dans les paramètres d'OSMV.
  - Vérifiez que votre application Discord est lancée en local et connectée.
  - Assurez-vous que le **Client ID** dans OSMV correspond à une application Discord valide créée sur le portail des développeurs.
