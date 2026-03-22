// src-rust/app.rs
// Application orchestrator — owns the provider + shared state updated on a background thread.
// The egui paint function reads from `AppState` (wrapped in Arc<Mutex>).

use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use crate::media::{self, SongInfo};
use crate::utils::{self, Settings};

// ── Shared state between background thread and GUI ────────────────────────────

#[derive(Default, Clone)]
pub struct AppState {
    pub song: SongInfo,
    pub settings: Settings,
    /// Decoded thumbnail image bytes (PNG/JPEG), ready for egui::ColorImage
    pub thumbnail_bytes: Option<Vec<u8>>,
    /// Key to detect song changes: "title|artist|art_url"
    pub last_song_key: String,
}

/// Creates the background polling thread and returns the shared state handle.
pub fn start_background(initial_settings: Settings) -> Arc<Mutex<AppState>> {
    let state = Arc::new(Mutex::new(AppState {
        settings: initial_settings,
        ..Default::default()
    }));

    let state_clone = Arc::clone(&state);

    thread::spawn(move || {
        let mut provider = media::create_provider();

        loop {
            let info = provider.current_song();

            let is_inactive = info.status == "closed"
                || info.status == "stopped"
                || info.title.is_empty();

            let (dynamic_color, audio_visualizer, last_key) = {
                let locked = state_clone.lock().unwrap();
                (
                    locked.settings.dynamic_color,
                    locked.settings.audio_visualizer,
                    locked.last_song_key.clone(),
                )
            };

            if is_inactive {
                utils::write_null_json();
                let mut locked = state_clone.lock().unwrap();
                locked.song = info;
                locked.thumbnail_bytes = None;
            } else {
                // Build cache key
                let key = format!("{}|{}|{}", info.title, info.artist, info.art_url);

                // Fetch thumbnail only when song changes
                let (thumb_b64, thumb_bytes) = if key != last_key {
                    let b64 = if !info.thumbnail_b64.is_empty() {
                        info.thumbnail_b64.clone()
                    } else if !info.art_url.is_empty() {
                        utils::load_thumbnail_as_base64(&info.art_url)
                    } else {
                        String::new()
                    };
                    let bytes = if !b64.is_empty() {
                        use base64::Engine;
                        base64::engine::general_purpose::STANDARD.decode(&b64).ok()
                    } else {
                        None
                    };
                    (b64, bytes)
                } else {
                    let locked = state_clone.lock().unwrap();
                    let b64 = locked.song.thumbnail_b64.clone();
                    let bytes = locked.thumbnail_bytes.clone();
                    (b64, bytes)
                };

                utils::write_json(
                    &info.title,
                    &info.artist,
                    &info.album,
                    &info.status,
                    &thumb_b64,
                    dynamic_color,
                    audio_visualizer,
                );

                let mut locked = state_clone.lock().unwrap();
                locked.last_song_key = key;
                locked.song = SongInfo {
                    thumbnail_b64: thumb_b64,
                    ..info
                };
                locked.thumbnail_bytes = thumb_bytes;
            }

            thread::sleep(Duration::from_millis(1000));
        }
    });

    state
}
