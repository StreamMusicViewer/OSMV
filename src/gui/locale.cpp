#include "locale.hpp"
#include <unordered_map>

namespace osmv::gui {

namespace {
struct LangPair {
    const char* fr;
    const char* en;
};

const std::unordered_map<std::string_view, LangPair>& dict() {
    static const std::unordered_map<std::string_view, LangPair> kDict = {
        // Navigation
        {"nav_now_playing", {"Now Playing", "Now Playing"}},
        {"nav_time_date", {"Temps & Date", "Time & Date"}},
        {"nav_timers", {"Minuteurs", "Timers"}},
        {"nav_sysinfo", {"Info Système", "System Info"}},
        {"nav_text_files", {"Texte & Fichiers", "Text & Files"}},
        {"nav_discord", {"Discord RPC", "Discord RPC"}},
        {"nav_twitch", {"Twitch", "Twitch"}},
        {"nav_settings", {"Paramètres", "Settings"}},
        {"nav_help", {"Aide & OBS", "Help & OBS"}},
        {"btn_headless", {"Mode Headless", "Minimize to Tray"}},
        {"btn_headless_tip", {"Ferme l'interface et libère toute sa RAM.\nLe daemon continue de tourner en arrière-plan.", "Closes the GUI and frees all its RAM.\nThe daemon keeps running in the background."}},
        {"btn_quit", {"Quitter OSMV", "Quit OSMV"}},
        {"daemon_ram", {"Daemon", "Daemon"}},
        {"gui_ram", {"Interface", "GUI"}},

        // Common UI
        {"enabled", {"Activé", "Enabled"}},
        {"disabled", {"Désactivé", "Disabled"}},
        {"enable_module", {"Activer ce module", "Enable this module"}},
        {"copy", {"Copier", "Copy"}},
        {"copied", {"Copié !", "Copied!"}},
        {"open_folder", {"Ouvrir le dossier", "Open Folder"}},
        {"save", {"Sauvegarder", "Save"}},
        {"saved_ok", {"Sauvegardé", "Saved"}},
        {"live_preview", {"Aperçu en direct", "Live output preview"}},
        {"file_path", {"Chemin du fichier OBS", "OBS file path"}},
        {"click_insert", {"Cliquer pour insérer un code :", "Click to insert format code:"}},
        {"interval_sec", {"Intervalle (s)", "Interval (s)"}},
        {"format", {"Format de sortie", "Output format"}},
        {"done_text", {"Texte quand terminé", "Text when finished"}},
        {"pad_zeros", {"Zéros initiaux (05:09)", "Zero-pad numbers (05:09)"}},

        // Now Playing
        {"np_title", {"Now Playing", "Now Playing"}},
        {"np_sub", {"Détecte la musique système et alimente OBS + exports texte Snaz.", "Detects system media playback and updates OBS + Snaz text exports."}},
        {"np_waiting", {"En attente de musique...", "Waiting for music..."}},
        {"np_playing", {"En lecture", "Playing"}},
        {"np_paused", {"En pause", "Paused"}},
        {"np_stopped", {"Arrêté", "Stopped"}},
        {"np_closed", {"Inactif", "Idle"}},
        {"np_album", {"Album", "Album"}},
        {"np_dyn_color", {"Couleur dynamique (OBS)", "Dynamic background color (OBS)"}},
        {"np_viz", {"Visualiseur audio (OBS)", "Audio visualizer (OBS)"}},
        {"np_obs_html", {"Source Navigateur OBS (index.html)", "OBS Browser Source (index.html)"}},
        {"np_snaz_exports", {"Exports texte & JSON (style Snaz)", "Snaz Text & JSON Exports"}},
        {"np_export_txt", {"Exporter TrackArtist.txt, TrackSong.txt et TrackAlbum.txt", "Export TrackArtist.txt, TrackSong.txt & TrackAlbum.txt"}},
        {"np_export_fmt", {"Exporter un fichier formaté (PlayingNow.txt)", "Export formatted file (PlayingNow.txt)"}},
        {"np_export_json", {"Exporter track-info.json (compatible Snaz)", "Export track-info.json (Snaz compatible)"}},
        {"np_clear_idle", {"Vider les fichiers texte quand la musique s'arrête", "Clear text files when music stops"}},
        {"np_fallback", {"Texte de repli", "Fallback text"}},

        // Time & Date
        {"td_title", {"Temps & Date", "Time & Date"}},
        {"td_sub", {"Génère Time.txt et Date.txt en temps réel pour vos sources texte OBS.", "Generates live Time.txt and Date.txt files for OBS text sources."}},
        {"time_card", {"Module Heure (Time.txt)", "Time Module (Time.txt)"}},
        {"time_ampm", {"Format 12 heures AM/PM ($tt)", "12-hour AM/PM format ($tt)"}},
        {"time_pc", {"Heure actuelle du PC", "Current PC time"}},
        {"date_card", {"Module Date (Date.txt)", "Date Module (Date.txt)"}},
        {"date_preset", {"Style de date ($date)", "Date format ($date)"}},
        {"date_tpl", {"Modèle de sortie", "Output template"}},

        // Timers
        {"tm_title", {"Minuteurs & Chronos", "Timers & Stopwatches"}},
        {"tm_sub", {"Compte à rebours vers une date, minuteur de pause et chronomètre.", "Target date countdown, break timer (Chrono Down), and stopwatch (Chrono Up)."}},
        {"cd_card", {"Countdown (Date cible)", "Countdown (Target Date)"}},
        {"cd_date", {"Date cible (A / M / J)", "Target date (Y / M / D)"}},
        {"cd_time", {"Heure cible (H / M / S)", "Target time (H / M / S)"}},
        {"cd_plus15", {"+15 min", "+15 min"}},
        {"cd_plus1h", {"+1 heure", "+1 hour"}},
        {"cd_tonight", {"Aujourd'hui 20:00", "Today 20:00"}},
        {"cdown_card", {"Chrono Down (Minuteur)", "Chrono Down (Timer)"}},
        {"cup_card", {"Chrono Up (Chronomètre)", "Chrono Up (Stopwatch)"}},
        {"duration_hms", {"Durée initiale (H / M / S)", "Initial duration (H / M / S)"}},
        {"start_hms", {"Départ (H / M / S)", "Start offset (H / M / S)"}},
        {"start", {"Démarrer", "Start"}},
        {"pause", {"Pause", "Pause"}},
        {"reset", {"Réinitialiser", "Reset"}},
        {"running", {"En cours", "Running"}},
        {"paused_lbl", {"En pause", "Paused"}},

        // System Info
        {"si_title", {"Informations Système", "System Info"}},
        {"si_sub", {"Moniteur matériel ultra-léger (CPU, RAM, processus, réseau) vers SystemInfo.txt.", "Ultra-lightweight hardware monitor (CPU, RAM, processes, network) to SystemInfo.txt."}},
        {"si_cpu", {"CPU", "CPU"}},
        {"si_ram", {"RAM Utilisée", "RAM Used"}},
        {"si_procs", {"Processus", "Processes"}},
        {"si_net", {"Réseau", "Network"}},

        // Textline & Dynamic Files
        {"tf_title", {"Texte Rotatif & Fichiers Dynamiques", "Textline Changer & Dynamic Files"}},
        {"tf_sub", {"Alternez automatiquement des messages texte et récupérez des fichiers texte/image via URL.", "Rotate text messages automatically and fetch live text/image files from HTTP URLs."}},
        {"tl_card", {"Textline Changer", "Textline Changer"}},
        {"tl_filename", {"Nom du fichier", "Output filename"}},
        {"tl_random", {"Ordre aléatoire", "Random order"}},
        {"tl_skip_empty", {"Ignorer les lignes vides", "Skip empty lines"}},
        {"tl_lines", {"Messages (1 par ligne)", "Messages (1 per line)"}},
        {"tl_next", {"Ligne suivante", "Next line"}},
        {"df_card", {"Dynamic Files (URL -> Fichier)", "Dynamic Files (URL -> File)"}},
        {"df_add", {"Ajouter une règle", "Add Rule"}},
        {"df_type_text", {"Texte", "Text"}},
        {"df_type_img", {"Image", "Image"}},
        {"df_headers", {"En-têtes HTTP (optionnel, Clé: Valeur)", "HTTP Headers (optional, Key: Value)"}},
        {"df_delete", {"Supprimer", "Delete"}},

        // Discord
        {"dc_title", {"Discord Rich Presence", "Discord Rich Presence"}},
        {"dc_sub", {"Affiche votre musique en cours ou un statut personnalisé sur votre profil Discord.", "Shows currently playing music or a custom status on your Discord profile."}},
        {"dc_connected", {"Connecté à Discord", "Connected to Discord"}},
        {"dc_waiting", {"En attente de Discord...", "Waiting for Discord..."}},
        {"dc_connecting", {"Connexion...", "Connecting..."}},
        {"dc_invalid", {"Client ID invalide", "Invalid Client ID"}},
        {"dc_client_id", {"Application Client ID", "Application Client ID"}},
        {"dc_dev_portal", {"Ouvrir Discord Developer Portal", "Open Discord Developer Portal"}},
        {"dc_use_music", {"Afficher la musique en cours automatiquement", "Automatically show currently playing song"}},
        {"dc_idle_sec", {"Activité par défaut / hors musique", "Default / Idle Activity"}},
        {"dc_details", {"Détails (ligne 1)", "Details (line 1)"}},
        {"dc_state", {"État (ligne 2)", "State (line 2)"}},
        {"dc_assets", {"Images & Badges (Discord Art Assets)", "Images & Status Badges (Art Assets)"}},
        {"dc_large_key", {"Grande image (fallback)", "Large image key (fallback)"}},
        {"dc_large_txt", {"Infobulle grande image", "Large image tooltip"}},
        {"dc_key_play", {"Badge Playing", "Playing badge key"}},
        {"dc_key_pause", {"Badge Paused", "Paused badge key"}},
        {"dc_key_stop", {"Badge Stopped", "Stopped badge key"}},
        {"dc_small_key", {"Petite image (idle)", "Small image key (idle)"}},
        {"dc_small_txt", {"Infobulle petite image", "Small image tooltip"}},

        // Twitch
        {"tw_title", {"Intégration Twitch (Helix)", "Twitch Integration (Helix)"}},
        {"tw_sub", {"Exportez vos viewers en direct (TwitchViewers.txt) et modifiez le titre/jeu du stream.", "Export live viewer count (TwitchViewers.txt) and update stream title/category."}},
        {"tw_creds", {"Connexion API Twitch", "Twitch API Credentials"}},
        {"tw_client_id", {"Twitch Client ID", "Twitch Client ID"}},
        {"tw_token", {"OAuth Access Token", "OAuth Access Token"}},
        {"tw_channel", {"Nom de la chaîne", "Channel username"}},
        {"tw_get_token", {"Générer un Token", "Get OAuth Token"}},
        {"tw_viewers_card", {"Compteur de Viewers (TwitchViewers.txt)", "Viewer Count (TwitchViewers.txt)"}},
        {"tw_offline_txt", {"Texte si hors ligne", "Offline text"}},
        {"tw_update_card", {"Modifier Titre & Jeu du Stream", "Update Stream Title & Game"}},
        {"tw_stream_title", {"Titre du stream", "Stream title"}},
        {"tw_search_game", {"Rechercher un jeu / catégorie", "Search game / category"}},
        {"tw_search_btn", {"Rechercher", "Search"}},
        {"tw_apply_btn", {"Appliquer sur Twitch", "Update Stream"}},
        {"tw_presets", {"Jeux favoris (raccourcis)", "Favorite Games (Presets)"}},
        {"tw_add_preset", {"Ajouter aux favoris", "Add to presets"}},

        // Settings
        {"set_title", {"Paramètres Généraux", "General Settings"}},
        {"set_sub", {"Personnalisez l'apparence, la langue, le dossier de sortie et le démarrage.", "Customize language, theme, output directory, and startup behavior."}},
        {"set_lang", {"Langue de l'interface", "Interface Language"}},
        {"set_theme", {"Thème", "Theme"}},
        {"set_dark", {"Sombre", "Dark"}},
        {"set_light", {"Clair", "Light"}},
        {"set_accent", {"Couleur d'accentuation", "Accent Color"}},
        {"set_outdir", {"Dossier des fichiers texte dynamiques", "Dynamic text files directory"}},
        {"set_outdir_hint", {"Laissez vide pour utiliser shared/dynamic_text par défaut.", "Leave empty to use shared/dynamic_text by default."}},
        {"set_reset_dir", {"Par défaut", "Reset Default"}},
        {"set_open_gui", {"Ouvrir l'interface automatiquement au lancement", "Open GUI automatically on launch"}},
        {"set_autostart", {"Lancer OSMV au démarrage du système (en arrière-plan)", "Start OSMV with system (in background)"}},

        // Help
        {"help_title", {"À propos d'OSMV & Guide OBS", "About OSMV & OBS Guide"}},
        {"help_sub", {"Moteur natif C++20 ultra-léger + modules Snaz intégrés.", "Ultra-lightweight native C++20 engine with integrated Snaz modules."}},
        {"help_desc", {"OSMV (OBS Stream Music Viewer) v3.0 combine un daemon C++ basse consommation et une interface moderne pour alimenter vos scènes OBS et votre Discord Rich Presence.", "OSMV (OBS Stream Music Viewer) v3.0 combines a tiny C++ background daemon with a clean interface for OBS overlays and Discord Rich Presence."}},
        {"help_obs_title", {"Démarrage rapide OBS Studio", "OBS Studio Quick Setup"}},
        {"help_obs_1", {"1. Widget Musique : Ajoutez une source Navigateur -> Fichier local -> shared/index.html (500 x 140).", "1. Music Widget: Add a Browser Source -> Local file -> shared/index.html (500 x 140)."}},
        {"help_obs_2", {"2. Textes dynamiques : Ajoutez une source Texte (GDI+ ou FreeType 2) -> Lire depuis un fichier -> sélectionnez Time.txt, Date.txt, SystemInfo.txt, etc.", "2. Dynamic Text: Add a Text source (GDI+ or FreeType 2) -> Read from file -> select Time.txt, Date.txt, SystemInfo.txt, etc."}},
        {"help_obs_3", {"3. Astuce RAM : Cliquez sur \"Mode Headless\" ou fermez cette fenêtre [X] pendant votre stream pour libérer la RAM graphique tout en gardant le daemon actif dans la barre des tâches.", "3. RAM Tip: Click \"Minimize to Tray\" or close this window [X] while streaming to free all GUI RAM while the tiny daemon keeps running in the system tray."}},
        {"help_mem_title", {"Empreinte mémoire en direct", "Live Memory Footprint"}}
    };
    return kDict;
}
} // namespace

const char* tr(const std::string& lang, std::string_view key) {
    const auto& d = dict();
    auto it = d.find(key);
    if (it == d.end()) return key.data();
    return (lang == "en") ? it->second.en : it->second.fr;
}

} // namespace osmv::gui
