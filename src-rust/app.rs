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
pub fn start_background(initial_settings: Settings, is_daemon: bool) -> Arc<Mutex<AppState>> {
    let state = Arc::new(Mutex::new(AppState {
        settings: initial_settings.clone(),
        ..Default::default()
    }));

    // Start Discord RPC thread ONLY if this is the daemon process
    let discord = if is_daemon {
        Some(start_discord_thread(initial_settings.discord.clone()))
    } else {
        None
    };

    let state_clone = Arc::clone(&state);

    thread::spawn(move || {
        run_poll_loop(state_clone, discord, is_daemon);
    });

    state
}

fn run_poll_loop(state: Arc<Mutex<AppState>>, discord: Option<DiscordHandle>, is_daemon: bool) {
    let mut provider = media::create_provider();

    let mut last_settings_mtime = std::time::SystemTime::UNIX_EPOCH;

    loop {
        // Hot-reload settings
        let settings_path = utils::settings_path();
        if let Ok(metadata) = std::fs::metadata(&settings_path) {
            if let Ok(mtime) = metadata.modified() {
                if mtime > last_settings_mtime {
                    last_settings_mtime = mtime;
                    let new_settings = utils::load_settings();
                    let mut locked = state.lock().unwrap();
                    locked.settings = new_settings;
                }
            }
        }

        let (now_playing_enabled, time_enabled, discord_enabled) = {
            let locked = state.lock().unwrap();
            (
                locked.settings.now_playing_enabled,
                locked.settings.time.enabled,
                locked.settings.discord.enabled,
            )
        };

        if !now_playing_enabled && !time_enabled && !discord_enabled {
            #[cfg(target_os = "linux")]
            unsafe {
                libc::malloc_trim(0);
            }
            thread::sleep(Duration::from_millis(1000));
            continue;
        }

        let info = provider.current_song();

        let is_inactive =
            info.status == "closed" || info.status == "stopped" || info.title.is_empty();

        let (now_playing_enabled, dynamic_color, audio_visualizer, last_key, discord_settings) = {
            let locked = state.lock().unwrap();
            (
                locked.settings.now_playing_enabled,
                locked.settings.dynamic_color,
                locked.settings.audio_visualizer,
                locked.last_song_key.clone(),
                locked.settings.discord.clone(),
            )
        };

        // ── Discord RPC update ────────────────────────────────────────────────
        if is_daemon && discord_settings.enabled {
            let (title, artist, art_url, status) = if !is_inactive {
                (
                    Some(info.title.clone()),
                    Some(info.artist.clone()),
                    Some(info.art_url.clone()),
                    info.status.clone(),
                )
            } else {
                (None, None, None, "idle".to_string())
            };
            if let Some(ref d) = discord {
                d.send(RpcCommand::Update {
                    settings: discord_settings,
                    title,
                    artist,
                    art_url,
                    status,
                });
            }
        }

        // ── JSON + state update ───────────────────────────────────────────────
        if !now_playing_enabled {
            if is_daemon {
                let json_path = utils::json_output_path();
                if json_path.exists() {
                    let _ = std::fs::remove_file(json_path);
                }
            }
            let mut locked = state.lock().unwrap();
            locked.song = info;
            locked.song.thumbnail_b64 = String::new();
            locked.thumbnail_bytes = None;
            locked.last_song_key = String::new();
        } else if is_inactive {
            if is_daemon {
                utils::write_null_json();
            }
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
                (
                    locked.song.thumbnail_b64.clone(),
                    locked.thumbnail_bytes.clone(),
                )
            };

            if is_daemon {
                utils::write_json(
                    &info.title,
                    &info.artist,
                    &info.album,
                    &info.status,
                    &thumb_b64,
                    dynamic_color,
                    audio_visualizer,
                );
            }

            let mut locked = state.lock().unwrap();
            locked.last_song_key = key;
            locked.song = SongInfo {
                thumbnail_b64: thumb_b64,
                ..info
            };
            locked.thumbnail_bytes = thumb_bytes;
        }

        // ── Time file generation ──────────────────────────────────────────────
        if is_daemon {
            let time_settings = {
                let locked = state.lock().unwrap();
                locked.settings.time.clone()
            };
            let dir = utils::shared_dir().join("dynamic_text");
            let file_path = dir.join("Time.txt");
            if time_settings.enabled {
                let (_, formatted) =
                    utils::format_time_string(&time_settings.format, time_settings.use_ampm);
                if !dir.exists() {
                    let _ = std::fs::create_dir_all(&dir);
                }
                let _ = std::fs::write(&file_path, formatted);
            } else if file_path.exists() {
                let _ = std::fs::remove_file(&file_path);
            }
        }

        #[cfg(target_os = "linux")]
        unsafe {
            libc::malloc_trim(0);
        }

        thread::sleep(Duration::from_millis(1000));
    }
}
