# Bienvenue sur la Documentation d'OSMV 🎵

> **OSMV (OBS Stream Music Viewer)** est une solution moderne, ultra-légère et personnalisable pour afficher votre musique en cours de lecture et votre présence Discord lors de vos streams OBS.

Ce site de documentation regroupe toutes les informations nécessaires pour installer, compiler, configurer et dépanner OSMV.

<p align="center">
  <img src="../assets/OSMV_logo.ico" alt="Logo OSMV" width="128" style="border-radius: 20px; box-shadow: 0 4px 20px rgba(0,0,0,0.5);" />
</p>

---

## ✨ Fonctionnalités Majeures

- **Architecture Bi-processus (Nouveau !)** : Un daemon d'arrière-plan ultra-optimisé en Rust (~15 Mo de RAM) pour l'écriture de fichiers et le traitement RPC Discord, combiné à une interface de configuration Qt 6 QML riche que vous pouvez fermer à tout moment pour libérer de la RAM.
- **Support Multiplateforme** : Compatible avec **Linux** (testé sous Arch/Hyprland, Ubuntu, KDE/Wayland et X11) et **Windows 10/11**.
- **Double Intégration** :
  - **Widget OBS** : Fichier HTML/CSS autonome avec animations fluides de transition d'album et de texte.
  - **Discord Rich Presence** : Affiche automatiquement ce que vous écoutez avec prise en charge des pochettes d'albums (via l'API iTunes).
- **Module Horloge (Time)** : Intégration d'un module d'affichage de l'heure hautement personnalisable avec formatage à la seconde.

---

## 🗺️ Plan de la Documentation

1. **[Installation & Dépendances](installation.md)** : Apprenez à installer les dépendances requises pour exécuter l'application pré-compilée ou coder dessus.
2. **[Guide de Configuration](configuration.md)** : Configurez l'affichage dans OBS, réglez l'intégration Discord RPC et personnalisez le module de temps.
3. **[Architecture Technique](architecture.md)** : Plongez sous le capot pour comprendre comment le daemon communique avec le GUI et comment la RAM a été optimisée de 378 Mo à 15 Mo.
4. **[Résolution de problèmes (Troubleshooting)](troubleshooting.md)** : Consultez les questions fréquentes et solutions pour les icônes de notification, les lecteurs non reconnus et la compilation.
