// src-rust/qml_interface.rs
// Pont CXX-Qt : expose OsmvEngine comme QObject au moteur QML Qt6.
// Ce module assure la synchronisation bidirectionnelle entre le thread
// de polling Rust (app.rs) et l'interface QML.

use crate::app::{start_background, AppState};
use crate::utils::{load_settings, save_settings};
use std::sync::{Arc, Mutex};

// ── Bridge CXX-Qt ─────────────────────────────────────────────────────────────

#[cxx_qt::bridge]
pub mod ffi {
    // ── Types Qt importés ─────────────────────────────────────────────────────
    unsafe extern "C++Qt" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    // ── OsmvEngine (QObject exposé à QML) ────────────────────────────────────
    unsafe extern "RustQt" {
        #[qobject]
        #[qml_element]

        // === Propriétés lecture en cours ===================================
        #[qproperty(QString, title)]
        #[qproperty(QString, artist)]
        #[qproperty(QString, album)]
        /// "playing" | "paused" | "stopped" | "closed"
        #[qproperty(QString, status)]
        /// Chemin vers le fichier temporaire de la pochette (file:// URL côté QML)
        #[qproperty(QString, thumbnail_path)]
        /// Chemin vers l'icône de logo OSMV
        #[qproperty(QString, logo_path)]

        // === Paramètres globaux ============================================
        #[qproperty(bool, dynamic_color)]
        #[qproperty(bool, audio_visualizer)]

        // === Configuration Discord RPC =====================================
        #[qproperty(bool, dc_enabled)]
        #[qproperty(bool, dc_use_music)]
        #[qproperty(QString, dc_client_id)]
        #[qproperty(QString, dc_details)]
        #[qproperty(QString, dc_state)]
        #[qproperty(QString, dc_large_key)]
        #[qproperty(QString, dc_large_text)]
        #[qproperty(QString, dc_key_playing)]
        #[qproperty(QString, dc_key_paused)]
        #[qproperty(QString, dc_key_stopped)]
        #[qproperty(QString, dc_small_key)]
        #[qproperty(QString, dc_small_text)]

        type OsmvEngine = super::OsmvEngineRust;

        // === Invokables (appelables depuis QML) ============================

        /// Sauvegarde les paramètres média (couleur dynamique, visualiseur)
        #[qinvokable]
        fn save_media_settings(self: Pin<&mut OsmvEngine>);

        /// Sauvegarde tous les paramètres Discord RPC
        #[qinvokable]
        fn save_discord_settings(self: Pin<&mut OsmvEngine>);

        /// Synchronise les propriétés QML depuis le thread Rust (appelé par Timer QML)
        #[qinvokable]
        fn poll(self: Pin<&mut OsmvEngine>);
    }
}

// ── État Rust interne de OsmvEngine ──────────────────────────────────────────

pub struct OsmvEngineRust {
    /// Handle vers l'état partagé avec le thread de polling
    app_state: Arc<Mutex<AppState>>,

    // Propriétés (miroir des qproperty)
    title:          cxx_qt_lib::QString,
    artist:         cxx_qt_lib::QString,
    album:          cxx_qt_lib::QString,
    status:         cxx_qt_lib::QString,
    thumbnail_path: cxx_qt_lib::QString,
    logo_path:      cxx_qt_lib::QString,

    dynamic_color:    bool,
    audio_visualizer: bool,

    dc_enabled:      bool,
    dc_use_music:    bool,
    dc_client_id:    cxx_qt_lib::QString,
    dc_details:      cxx_qt_lib::QString,
    dc_state:        cxx_qt_lib::QString,
    dc_large_key:    cxx_qt_lib::QString,
    dc_large_text:   cxx_qt_lib::QString,
    dc_key_playing:  cxx_qt_lib::QString,
    dc_key_paused:   cxx_qt_lib::QString,
    dc_key_stopped:  cxx_qt_lib::QString,
    dc_small_key:    cxx_qt_lib::QString,
    dc_small_text:   cxx_qt_lib::QString,
}

impl Default for OsmvEngineRust {
    fn default() -> Self {
        let settings = load_settings();
        let dc = &settings.discord;

        Self {
            app_state:        start_background(settings.clone()),
            title:            cxx_qt_lib::QString::from("Waiting for music..."),
            artist:           cxx_qt_lib::QString::from(""),
            album:            cxx_qt_lib::QString::from(""),
            status:           cxx_qt_lib::QString::from("closed"),
            thumbnail_path:   cxx_qt_lib::QString::from(""),
            logo_path: {
                let mut logo_path = std::env::current_dir().unwrap_or_default();
                logo_path.push("assets");
                logo_path.push("OSMV_logo.ico");
                if !logo_path.exists() {
                    if let Ok(exe_path) = std::env::current_exe() {
                        if let Some(exe_dir) = exe_path.parent() {
                            let mut alt_path = exe_dir.to_path_buf();
                            alt_path.push("assets");
                            alt_path.push("OSMV_logo.ico");
                            if alt_path.exists() {
                                logo_path = alt_path;
                            }
                        }
                    }
                }
                cxx_qt_lib::QString::from(&logo_path.to_string_lossy().to_string())
            },
            dynamic_color:    settings.dynamic_color,
            audio_visualizer: settings.audio_visualizer,
            dc_enabled:       dc.enabled,
            dc_use_music:     dc.use_music_when_playing,
            dc_client_id:     cxx_qt_lib::QString::from(&dc.client_id),
            dc_details:       cxx_qt_lib::QString::from(&dc.custom_details),
            dc_state:         cxx_qt_lib::QString::from(&dc.custom_state),
            dc_large_key:     cxx_qt_lib::QString::from(&dc.large_image_key),
            dc_large_text:    cxx_qt_lib::QString::from(&dc.large_image_text),
            dc_key_playing:   cxx_qt_lib::QString::from(&dc.status_key_playing),
            dc_key_paused:    cxx_qt_lib::QString::from(&dc.status_key_paused),
            dc_key_stopped:   cxx_qt_lib::QString::from(&dc.status_key_stopped),
            dc_small_key:     cxx_qt_lib::QString::from(&dc.small_image_key),
            dc_small_text:    cxx_qt_lib::QString::from(&dc.small_image_text),
        }
    }
}

// ── Implémentation des invokables ─────────────────────────────────────────────

impl ffi::OsmvEngine {
    /// Synchronise les propriétés QML avec l'état le plus récent du thread Rust.
    /// Appelé toutes les secondes depuis le Timer QML.
    fn poll(mut self: std::pin::Pin<&mut Self>) {
        // Récupérer un snapshot atomique de l'état
        let (new_title, new_artist, new_album, new_status, new_dyn, new_viz, song_key, thumb_bytes) = {
            let locked = self.app_state.lock().unwrap();
            let song = &locked.song;
            (
                if song.title.is_empty() {
                    "Waiting for music...".to_string()
                } else {
                    song.title.clone()
                },
                song.artist.clone(),
                song.album.clone(),
                song.status.clone(),
                locked.settings.dynamic_color,
                locked.settings.audio_visualizer,
                locked.last_song_key.clone(),
                locked.thumbnail_bytes.clone(),
            )
        };

        // Calculer le chemin du fichier temporaire de la pochette.
        // Le nom est basé sur un hash du song_key → même chanson = même fichier,
        // chanson différente = nouveau chemin → QML recharge l'image + animation.
        let new_thumb_path = compute_thumb_path(&song_key, thumb_bytes.as_deref());

        // Convertir en QString
        let q_title  = cxx_qt_lib::QString::from(&new_title);
        let q_artist = cxx_qt_lib::QString::from(&new_artist);
        let q_album  = cxx_qt_lib::QString::from(&new_album);
        let q_status = cxx_qt_lib::QString::from(&new_status);
        let q_thumb  = cxx_qt_lib::QString::from(&new_thumb_path);

        // Ne mettre à jour que si les données ont changé (évite des redraws inutiles)
        if *self.as_ref().title()  != q_title  { self.as_mut().set_title(q_title);   }
        if *self.as_ref().artist() != q_artist  { self.as_mut().set_artist(q_artist); }
        if *self.as_ref().album()  != q_album   { self.as_mut().set_album(q_album);   }
        if *self.as_ref().status() != q_status  { self.as_mut().set_status(q_status); }
        if *self.as_ref().thumbnail_path() != q_thumb {
            self.as_mut().set_thumbnail_path(q_thumb);
        }
        if *self.as_ref().dynamic_color()   != new_dyn { self.as_mut().set_dynamic_color(new_dyn);     }
        if *self.as_ref().audio_visualizer() != new_viz { self.as_mut().set_audio_visualizer(new_viz); }
    }

    /// Persiste les paramètres média dans settings.json
    fn save_media_settings(self: std::pin::Pin<&mut Self>) {
        let mut locked = self.app_state.lock().unwrap();
        locked.settings.dynamic_color    = *self.as_ref().dynamic_color();
        locked.settings.audio_visualizer = *self.as_ref().audio_visualizer();
        save_settings(&locked.settings);
    }

    /// Persiste les paramètres Discord dans settings.json
    fn save_discord_settings(self: std::pin::Pin<&mut Self>) {
        let mut locked = self.app_state.lock().unwrap();
        let dc = &mut locked.settings.discord;
        dc.enabled                = *self.as_ref().dc_enabled();
        dc.use_music_when_playing = *self.as_ref().dc_use_music();
        dc.client_id              = self.as_ref().dc_client_id().to_string();
        dc.custom_details         = self.as_ref().dc_details().to_string();
        dc.custom_state           = self.as_ref().dc_state().to_string();
        dc.large_image_key        = self.as_ref().dc_large_key().to_string();
        dc.large_image_text       = self.as_ref().dc_large_text().to_string();
        dc.status_key_playing     = self.as_ref().dc_key_playing().to_string();
        dc.status_key_paused      = self.as_ref().dc_key_paused().to_string();
        dc.status_key_stopped     = self.as_ref().dc_key_stopped().to_string();
        dc.small_image_key        = self.as_ref().dc_small_key().to_string();
        dc.small_image_text       = self.as_ref().dc_small_text().to_string();
        save_settings(&locked.settings);
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Calcule le chemin du fichier temporaire de la pochette et l'écrit si besoin.
/// Retourne le chemin absolu (String) si une image est disponible, sinon "".
fn compute_thumb_path(song_key: &str, thumb_bytes: Option<&[u8]>) -> String {
    let Some(bytes) = thumb_bytes else { return String::new(); };
    if bytes.is_empty() { return String::new(); }

    // Hash du song_key → nom de fichier unique par chanson
    let hash = {
        use std::hash::{Hash, Hasher};
        let mut h = std::collections::hash_map::DefaultHasher::new();
        song_key.hash(&mut h);
        h.finish()
    };

    let path = std::env::temp_dir().join(format!("osmv_thumb_{:016x}.jpg", hash));

    // N'écrire que si le fichier n'existe pas encore (même chanson = pas de re-write)
    if !path.exists() {
        let _ = std::fs::write(&path, bytes);
    }

    path.to_string_lossy().into_owned()
}
