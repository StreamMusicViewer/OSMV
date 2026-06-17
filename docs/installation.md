# Installation et Dépendances 🛠️

Pour exécuter le binaire pré-compilé ou configurer votre environnement de développement afin de compiler et coder sur OSMV, vous devez installer les dépendances listées ci-dessous.

---

## 📋 Dépendances Système requises

### 🐧 Linux (Arch Linux, Hyprland, etc.)

Sous Linux, vous aurez besoin de **Qt 6**, **Clang**, **GTK 3** (pour le tray icon) et **D-Bus** (pour la détection de musique MPRIS).

Installez les paquets suivants via votre gestionnaire de paquets :

#### Arch Linux (Recommandé)
```bash
sudo pacman -S rust clang qt6-declarative qt6-languageserver gtk3 dbus pkgconf build-essential
```

#### Ubuntu / Debian (22.04 / 24.04+)
```bash
sudo apt update
sudo apt install cargo rustc clang libclang-dev qt6-base-dev qt6-declarative-dev libgtk-3-dev libdbus-1-dev pkg-config build-essential
```

#### Fedora
```bash
sudo dnf install cargo rust clang-devel qt6-qtdeclarative-devel gtk3-devel dbus-devel pkgconf-pkg-config gcc-c++
```

---

### 🪟 Windows

Pour compiler ou coder sous Windows, assurez-vous d'avoir :

1. **Rustup** : Installez-le via [rustup.rs](https://rustup.rs). Utilisez le toolchain stable MSVC (`stable-x86_64-pc-windows-msvc`).
2. **Visual Studio C++ Build Tools** : Requis par Rust pour compiler les liaisons C++. Cochez l'option "Développement Desktop en C++".
3. **Clang / LLVM** : Requis par CXX-Qt pour l'analyse syntaxique C++. Installez-le via `choco install llvm` ou téléchargez l'installeur LLVM officiel. **Ajoutez LLVM à votre variable d'environnement PATH**.
4. **Qt 6** : Téléchargez l'installateur open-source Qt et installez Qt 6.6 (ou supérieur) avec le module **Qt 6 Declarative (QML)**. Ajoutez le chemin `bin` de Qt à votre variable d'environnement `PATH` (ex: `C:\Qt\6.6.x\msvc2019_64\bin`).

---

## 🏗️ Compiler depuis les sources

### Linux
Exécutez simplement le script de compilation :
```bash
chmod +x linux/build_rust.sh
./linux/build_rust.sh
```
Ce script compile l'application en mode `release` optimisé et copie le binaire final dans le dossier racine.

### Windows
Exécutez le script batch dans votre terminal (PowerShell ou CMD avec privilèges de développeur VS) :
```cmd
windows\compile_rust.bat
```

---

## ⚡ Lancement de l'application

Une fois compilé, vous pouvez exécuter le binaire `osmv` situé à la racine du projet :

- **Démarrage par défaut (Daemon + GUI)** :
  ```bash
  ./osmv
  ```
  Le daemon démarre, place l'icône dans le tray, et lance automatiquement le GUI de configuration.
  
- **Démarrage en arrière-plan direct (sans GUI)** :
  Si l'application est configurée pour démarrer avec votre système (ex: dans Hyprland ou via autostart), lancez-la simplement via `./osmv`. Si une instance tourne déjà, relancer `./osmv` appellera le GUI de configuration sans créer de doublon.
