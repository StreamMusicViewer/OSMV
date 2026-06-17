# Résolution de problèmes (Troubleshooting) 🛠

Ce guide vous aide à résoudre les erreurs courantes de compilation, d'intégration ou d'affichage rencontrées avec OSMV.

---

## 🏗 Problèmes liés à la compilation (Compilation)

### `cxx-qt-build` ne compile pas ou renvoie des erreurs C++
- **Cause** : Votre compilateur C++ ou les en-têtes de développement Qt 6 ne sont pas présents ou mal configurés.
- **Solutions** :
  - Assurez-vous que les outils de compilation sont installés : `build-essential` et `gcc-c++` sous Linux, ou le module C++ de Visual Studio sous Windows.
  - CXX-Qt a besoin de `clang` et `libclang` pour analyser les liaisons Rust/C++. Vérifiez que la variable d'environnement `LIBCLANG_PATH` pointe bien vers le dossier contenant `libclang.so` (Linux) ou `libclang.dll` (Windows) si l'analyseur échoue automatiquement.

### Erreurs de compilation de la crate `tray-icon` sous Linux
- **Cause** : Les bibliothèques de développement GTK 3 et D-Bus sont manquantes.
- **Solution** : Installez `libgtk-3-dev` et `libdbus-1-dev` (sur les distributions basées sur Debian/Ubuntu) ou `gtk3-devel` et `dbus-devel` (sur Fedora/RedHat).

---

## 🐧 Problèmes spécifiques à Linux & Wayland (ex: Arch + Hyprland)

### L'icône dans la barre des tâches n'apparaît pas
- **Cause** : Wayland n'implémente pas nativement les icônes de notification (System Tray). Il faut un protocole de type KStatusNotifierItem (SNI).
- **Solutions** :
  - **Sur Hyprland / Waybar** : Le module `tray` doit être activé dans votre configuration Waybar. Ajoutez la ligne `"tray"` dans vos modules, et assurez-vous d'avoir installé `libappindicator-gtk3` ou `libayatana-appindicator3`.
  - **Sur GNOME** : Installez l'extension [AppIndicator Support](https://extensions.gnome.org/extension/615/appindicator-support/).

### Lancer l'application au démarrage de Hyprland
Pour lancer automatiquement OSMV en arrière-plan (sans afficher l'interface graphique de configuration) lors du démarrage de votre session Hyprland, ajoutez la ligne suivante à votre fichier `hyprland.conf` :
```text
exec-once = /chemin/vers/votre/dossier/OSMV/osmv
```
Comme l'application est protégée par un garde de démarrage unique (Single Instance Guard), si vous tentez de la relancer plus tard (en cliquant sur un raccourci ou via le terminal), cela ouvrira simplement l'interface de configuration sans perturber le daemon en cours.

---

## 🪟 Problèmes spécifiques à Windows

### Erreur : Le widget OBS affiche "Waiting for music..." alors que Spotify ou Apple Music tourne
- **Solutions** :
  - Vérifiez dans l'interface d'OSMV que le module **Now Playing** est sur **ON**.
  - Si vous utilisez Spotify Desktop, assurez-vous de ne pas être en session privée.
  - Si vous utilisez Spotify ou YouTube dans votre navigateur (Chrome/Firefox/Edge), vérifiez que l'intégration du lecteur système est activée (vérifiez le flag `#hardware-media-key-handling` dans votre navigateur si les touches média de votre clavier n'ont pas d'effet).

---

## 🛑 Relancer proprement l'application

Si pour une raison quelconque l'application ne répond plus ou que l'icône de notification a disparu mais que le processus tourne en tâche de fond :
- **Sous Linux** :
  ```bash
  killall osmv
  ```
- **Sous Windows** :
  Ouvrez le Gestionnaire des tâches, cherchez `osmv.exe`, faites un clic droit puis **Fin de tâche**.

Puis, relancez simplement `./osmv` (ou `OSMV.exe`).
