// src-rust/app.rs
// Application orchestrator — poll loop + Discord RPC driver.

use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use crate::discord::{start_discord_thread, DiscordHandle, RpcCommand};
use crate::media::{self, SongInfo};
use crate::utils::{self, Settings};

// ── Shared state ──────────────────────────────────────────────────────────────

#[derive(Default, Clone)]
pub struct AppState {
    pub song: SongInfo,
    pub settings: Settings,
    pub thumbnail_bytes: Option<Vec<u8>>,
    pub last_song_key: String,
}

/// Creates the background polling thread and returns the shared state handle.
pub fn start_background(initial_settings: Settings) -> Arc<Mutex<AppState>> {
    let state = Arc::new(Mutex::new(AppState {
        settings: initial_settings.clone(),
        ..Default::default()
    }));

    // Start Discord RPC thread
    let discord = start_discord_thread(initial_settings.discord.clone());

    let state_clone = Arc::clone(&state);

    thread::spawn(move || {
        run_poll_loop(state_clone, discord);
    });

    state
}

fn run_poll_loop(state: Arc<Mutex<AppState>>, discord: DiscordHandle) {
    let mut provider = media::create_provider();

    loop {
        let info = provider.current_song();

        let is_inactive = info.status == "closed"
            || info.status == "stopped"
            || info.title.is_empty();

        let (dynamic_color, audio_visualizer, last_key, discord_settings) = {
            let locked = state.lock().unwrap();
            (
                locked.settings.dynamic_color,
                locked.settings.audio_visualizer,
                locked.last_song_key.clone(),
                locked.settings.discord.clone(),
            )
        };

        // ── Discord RPC update ────────────────────────────────────────────────
        if discord_settings.enabled {
            let (title, artist) = if !is_inactive {
                (Some(info.title.clone()), Some(info.artist.clone()))
            } else {
                (None, None)
            };
            discord.send(RpcCommand::Update {
                settings: discord_settings,
                title,
                artist,
            });
        }

        // ── JSON + state update ───────────────────────────────────────────────
        if is_inactive {
            utils::write_null_json();
            let mut locked = state.lock().unwrap();
            locked.song = info;
            locked.thumbnail_bytes = None;
        } else {
            let key = format!("{}|{}|{}", info.title, info.artist, info.art_url);

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
                let locked = state.lock().unwrap();
                (locked.song.thumbnail_b64.clone(), locked.thumbnail_bytes.clone())
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

            let mut locked = state.lock().unwrap();
            locked.last_song_key = key;
            locked.song = SongInfo { thumbnail_b64: thumb_b64, ..info };
            locked.thumbnail_bytes = thumb_bytes;
        }

        thread::sleep(Duration::from_millis(1000));
    }
}
