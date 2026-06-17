// shared/qml/OsmvLocale.qml
// Dictionnaire de traduction (fr/en) pour l'interface OSMV.
import QtQuick 2.15

QtObject {
    // Langue par défaut : "fr". Peut être changée à "en".
    property string currentLanguage: "fr"

    // Fonction de traduction simple
    function t(key) {
        var dict = currentLanguage === "en" ? en : fr;
        return dict[key] !== undefined ? dict[key] : key;
    }

    // Dictionnaire Français
    readonly property var fr: ({
        "tab_now_playing": "Now Playing",
        "tab_discord_rpc": "Discord RPC",
        "tab_settings": "Paramètres",
        "tab_help": "Aide",

        // Vue Now Playing
        "status_playing": "En lecture",
        "status_paused": "En pause",
        "status_stopped": "Arrêté",
        "status_waiting": "En attente de musique...",
        "status_waiting_short": "En attente",
        "album_prefix": "Album : ",
        "match_cover": "Match couleur de la pochette",
        "audio_viz": "Visualiseur audio (beta)",

        // Vue Discord
        "dc_enable": "Activer la présence Discord",
        "dc_on": "Discord Rich Presence  ON",
        "dc_off": "Discord Rich Presence  OFF",
        "dc_section_client": "Application Client ID",
        "dc_hint_client": "Créez une app sur discord.com/developers et copiez l'ID",
        "dc_section_music": "Intégration Musique",
        "dc_music_cb": "Afficher la chanson en cours quand de la musique joue",
        "dc_music_hint": "Sans musique, l'activité personnalisée ci-dessous est affichée.",
        "dc_section_default": "Activité par Défaut / Fallback",
        "dc_hint_default": "Affichée quand aucune musique ne joue",
        "dc_details_label": "Détails (ligne 1)",
        "dc_state_label": "State (ligne 2)",
        "dc_section_large": "Pochette (grande image)",
        "dc_hint_large": "Clé de secours quand aucune pochette n'est disponible",
        "dc_large_key_label": "Clé grande image (fallback)",
        "dc_large_text_label": "Tooltip grande image",
        "dc_section_status": "Icônes de Statut (petite image)",
        "dc_hint_status": "Uploadez sur Discord App Assets avec ces noms exacts",
        "dc_section_small": "Petite Image Personnalisée / Idle",
        "dc_hint_small": "Affichée quand aucune musique ne joue",
        "dc_small_key_label": "Clé petite image",
        "dc_small_text_label": "Tooltip petite image",
        "btn_save": "Sauvegarder",
        "feedback_saved": "✓ Sauvegardé",

        // Vue Settings
        "set_title": "Paramètres Généraux",
        "set_lang_label": "Langue de l'interface",
        "set_theme_label": "Thème de l'application",
        "set_theme_mode_system": "Système",
        "set_theme_mode_dark": "Sombre",
        "set_theme_mode_light": "Clair",
        "set_theme_active": "Thème système détecté : ",
        "set_theme_dark": "Sombre",
        "set_theme_light": "Clair",

        // Vue Help
        "help_title": "À Propos d'OSMV",
        "help_version": "Version",
        "help_desc": "OSMV (OBS Stream Music Viewer) est un outil léger pour afficher la musique en cours de lecture dans OBS et la synchroniser avec votre Discord Rich Presence.",
        "help_author": "Développé par le projet StreamMusicViewer",
        "help_license": "Sous licence MIT / Apache 2.0"
    })

    // Dictionnaire Anglais
    readonly property var en: ({
        "tab_now_playing": "Now Playing",
        "tab_discord_rpc": "Discord RPC",
        "tab_settings": "Settings",
        "tab_help": "Help",

        // Now Playing View
        "status_playing": "Playing",
        "status_paused": "Paused",
        "status_stopped": "Stopped",
        "status_waiting": "Waiting for music...",
        "status_waiting_short": "Waiting",
        "album_prefix": "Album: ",
        "match_cover": "Match cover art color",
        "audio_viz": "Audio visualizer (beta)",

        // Discord View
        "dc_enable": "Enable Discord Rich Presence",
        "dc_on": "Discord Rich Presence  ON",
        "dc_off": "Discord Rich Presence  OFF",
        "dc_section_client": "Application Client ID",
        "dc_hint_client": "Create an app on discord.com/developers and copy the ID",
        "dc_section_music": "Music Integration",
        "dc_music_cb": "Show currently playing song as activity",
        "dc_music_hint": "Without music, the custom activity below is shown.",
        "dc_section_default": "Default / Fallback Activity",
        "dc_hint_default": "Shown when no music is playing",
        "dc_details_label": "Details (line 1)",
        "dc_state_label": "State (line 2)",
        "dc_section_large": "Cover Art (large image)",
        "dc_hint_large": "Fallback key when no cover art is available",
        "dc_large_key_label": "Large image key (fallback)",
        "dc_large_text_label": "Large image tooltip",
        "dc_section_status": "Status Icons (small image)",
        "dc_hint_status": "Upload to Discord App Assets with these exact names",
        "dc_section_small": "Custom Small Image / Idle",
        "dc_hint_small": "Shown when no music is playing",
        "dc_small_key_label": "Small image key",
        "dc_small_text_label": "Small image tooltip",
        "btn_save": "Save Settings",
        "feedback_saved": "✓ Saved",

        // Settings View
        "set_title": "General Settings",
        "set_lang_label": "Interface Language",
        "set_theme_label": "Application Theme",
        "set_theme_mode_system": "System",
        "set_theme_mode_dark": "Dark",
        "set_theme_mode_light": "Light",
        "set_theme_active": "System theme detected: ",
        "set_theme_dark": "Dark",
        "set_theme_light": "Light",

        // Help View
        "help_title": "About OSMV",
        "help_version": "Version",
        "help_desc": "OSMV (OBS Stream Music Viewer) is a lightweight tool to display currently playing music in OBS and synchronize it with your Discord Rich Presence.",
        "help_author": "Developed by the StreamMusicViewer project",
        "help_license": "Under MIT / Apache 2.0 license"
    })
}
