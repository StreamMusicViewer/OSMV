// src-rust/discord.rs
// Discord Rich Presence manager.
//
// Large image strategy (in priority order):
//   1. iTunes Search API  — fetch cover art URL by artist + title (HTTPS, public)
//   2. art_url from media — only if it's an HTTPS URL
//   3. User's fallback large_image_key (Discord Art Asset)
//
// Small image (bottom-right status badge):
//   Playing  -> status_key_playing  (default "playing")
//   Paused   -> status_key_paused   (default "paused")
//   Stopped  -> status_key_stopped  (default "stopped")
//   Idle     -> small_image_key    (user-configured)

use std::collections::HashMap;
use std::sync::mpsc::{self, Receiver, Sender};
use std::thread;
use std::time::Duration;

use discord_presence::Client;

use crate::utils::DiscordSettings;

// ── Public API ────────────────────────────────────────────────────────────────

pub enum RpcCommand {
    Update {
        settings: DiscordSettings,
        title:    Option<String>,
        artist:   Option<String>,
        art_url:  Option<String>,
        status:   String,
    },
    Clear,
    Shutdown,
}

pub struct DiscordHandle {
    tx: Sender<RpcCommand>,
}
impl DiscordHandle {
    pub fn send(&self, cmd: RpcCommand) { let _ = self.tx.send(cmd); }
}

pub fn start_discord_thread(initial_settings: DiscordSettings) -> DiscordHandle {
    let (tx, rx) = mpsc::channel();
    thread::spawn(move || run_discord_loop(rx, initial_settings));
    DiscordHandle { tx }
}

// ── Internal loop ─────────────────────────────────────────────────────────────

fn run_discord_loop(rx: Receiver<RpcCommand>, _initial: DiscordSettings) {
    let mut client: Option<Client> = None;
    let mut current_client_id = String::new();
    // Cache: "title|artist" -> HTTPS cover URL (avoids repeated API calls)
    let mut cover_cache: HashMap<String, String> = HashMap::new();

    loop {
        let cmd = match rx.recv_timeout(Duration::from_secs(5)) {
            Ok(c)  => c,
            Err(_) => continue,
        };

        match cmd {
            RpcCommand::Shutdown => break,

            RpcCommand::Clear => {
                if let Some(ref mut c) = client { let _ = c.clear_activity(); }
            }

            RpcCommand::Update { settings, title, artist, art_url, status } => {
                // Toggle off
                if !settings.enabled {
                    if let Some(ref mut c) = client { let _ = c.clear_activity(); }
                    client = None;
                    current_client_id.clear();
                    continue;
                }

                // (Re)connect if client ID changed
                if settings.client_id != current_client_id || client.is_none() {
                    if let Ok(id) = settings.client_id.parse::<u64>() {
                        if let Some(ref mut old) = client { let _ = old.clear_activity(); }
                        let mut c = Client::new(id);
                        c.start();
                        thread::sleep(Duration::from_millis(400));
                        current_client_id = settings.client_id.clone();
                        client = Some(c);
                    } else {
                        continue;
                    }
                }

                let Some(ref mut c) = client else { continue };

                let music_active = title.is_some() && settings.use_music_when_playing;

                // ── Text lines ────────────────────────────────────────────────
                let (details, state) = if music_active {
                    (
                        title.as_deref().unwrap_or("").to_string(),
                        format!("by {}", artist.as_deref().unwrap_or("")),
                    )
                } else {
                    (settings.custom_details.clone(), settings.custom_state.clone())
                };

                // ── Large image ───────────────────────────────────────────────
                let large_image = if music_active {
                    let t = title.as_deref().unwrap_or("");
                    let a = artist.as_deref().unwrap_or("");
                    let cache_key = format!("{}|{}", t, a);

                    // 1. Check cache first
                    if let Some(cached) = cover_cache.get(&cache_key) {
                        cached.clone()
                    } else {
                        // 2. Try iTunes API
                        let itunes_url = fetch_itunes_cover(t, a);
                        if let Some(ref url) = itunes_url {
                            cover_cache.insert(cache_key.clone(), url.clone());
                            url.clone()
                        } else {
                            // 3. Try the media provider's art_url if HTTPS
                            let from_provider = art_url
                                .as_deref()
                                .filter(|u| u.starts_with("https://"))
                                .map(str::to_string);
                            let fallback = from_provider.unwrap_or_else(|| {
                                if !settings.large_image_key.is_empty() {
                                    settings.large_image_key.clone()
                                } else {
                                    "osmv_logo".to_string()
                                }
                            });
                            // Cache the fallback so we don't retry every second
                            cover_cache.insert(cache_key, fallback.clone());
                            fallback
                        }
                    }
                } else {
                    // Idle / custom mode
                    if !settings.large_image_key.is_empty() {
                        settings.large_image_key.clone()
                    } else {
                        "osmv_logo".to_string()
                    }
                };

                let large_text = if music_active {
                    artist.as_deref().unwrap_or("").to_string()
                } else {
                    settings.large_image_text.clone()
                };

                // ── Small image (status badge) ────────────────────────────────
                let (small_key, small_text): (String, String) = if music_active {
                    match status.as_str() {
                        "playing" => (settings.status_key_playing.clone(), "Playing".into()),
                        "paused"  => (settings.status_key_paused.clone(),  "Paused".into()),
                        "stopped" => (settings.status_key_stopped.clone(), "Stopped".into()),
                        _         => (String::new(), String::new()),
                    }
                } else {
                    (settings.small_image_key.clone(), settings.small_image_text.clone())
                };

                // ── Set activity ──────────────────────────────────────────────
                let _ = c.set_activity(|a| {
                    let a = a.details(&details).state(&state);
                    a.assets(|assets| {
                        let assets = assets
                            .large_image(&large_image)
                            .large_text(&large_text);
                        if !small_key.is_empty() {
                            assets.small_image(&small_key).small_text(&small_text)
                        } else {
                            assets
                        }
                    })
                });
            }
        }
    }

    if let Some(ref mut c) = client { let _ = c.clear_activity(); }
}

// ── iTunes Search API ─────────────────────────────────────────────────────────

/// Query the iTunes Search API (free, no auth) and return a 500x500 cover art URL.
fn fetch_itunes_cover(title: &str, artist: &str) -> Option<String> {
    // Simple URL-safe encoding: replace spaces with +, strip special chars
    let query = format!("{} {}", title, artist)
        .chars()
        .map(|c| if c.is_alphanumeric() || c == ' ' { c } else { ' ' })
        .collect::<String>()
        .split_whitespace()
        .collect::<Vec<_>>()
        .join("+");

    let url = format!(
        "https://itunes.apple.com/search?term={}&entity=song&limit=1&media=music",
        query
    );

    let resp = ureq::get(&url)
        .set("User-Agent", "OSMV/2.0")
        .call()
        .ok()?;

    let json: serde_json::Value = resp.into_json().ok()?;

    let artwork = json["results"]
        .as_array()?
        .first()?
        .get("artworkUrl100")?
        .as_str()?;

    // Upscale thumbnail: 100x100bb -> 500x500bb
    Some(artwork.replace("100x100bb", "500x500bb"))
}
