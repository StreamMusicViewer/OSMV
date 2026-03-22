// src-rust/discord.rs
// Discord Rich Presence manager.
// Runs on a dedicated thread; receives commands via a channel.

use std::sync::mpsc::{self, Receiver, Sender};
use std::thread;
use std::time::Duration;

use discord_presence::Client;

use crate::utils::DiscordSettings;

/// Commands sent from the main app to the Discord thread.
pub enum RpcCommand {
    /// Update the activity based on current song + settings snapshot
    Update {
        settings: DiscordSettings,
        /// None = no music playing
        title: Option<String>,
        artist: Option<String>,
    },
    /// Clear the activity (disconnect / no presence)
    Clear,
    /// Shutdown the thread
    Shutdown,
}

/// Handle returned to the main app for communicating with the Discord thread.
pub struct DiscordHandle {
    tx: Sender<RpcCommand>,
}

impl DiscordHandle {
    pub fn send(&self, cmd: RpcCommand) {
        let _ = self.tx.send(cmd);
    }
}

/// Spawn the Discord RPC background thread. Returns immediately.
pub fn start_discord_thread(initial_settings: DiscordSettings) -> DiscordHandle {
    let (tx, rx): (Sender<RpcCommand>, Receiver<RpcCommand>) = mpsc::channel();

    thread::spawn(move || {
        run_discord_loop(rx, initial_settings);
    });

    DiscordHandle { tx }
}

// ── Discord thread loop ───────────────────────────────────────────────────────

fn run_discord_loop(rx: Receiver<RpcCommand>, mut settings: DiscordSettings) {
    let mut client: Option<Client> = None;
    let mut current_client_id = String::new();

    loop {
        // Wait for next command (blocking)
        let cmd = match rx.recv_timeout(Duration::from_secs(5)) {
            Ok(c) => c,
            Err(_) => continue, // timeout — keep waiting
        };

        match cmd {
            RpcCommand::Shutdown => break,

            RpcCommand::Clear => {
                if let Some(ref mut c) = client {
                    let _ = c.clear_activity();
                }
            }

            RpcCommand::Update { settings: new_settings, title, artist } => {
                // If client ID changed or toggled on, reconnect
                let need_reconnect = new_settings.enabled
                    && (new_settings.client_id != current_client_id || client.is_none());

                // Toggle off — disconnect
                if !new_settings.enabled {
                    if let Some(ref mut c) = client {
                        let _ = c.clear_activity();
                    }
                    client = None;
                    current_client_id.clear();
                    settings = new_settings;
                    continue;
                }

                // Connect if needed
                if need_reconnect {
                    if let Ok(id) = new_settings.client_id.parse::<u64>() {
                        let mut c = Client::new(id);
                        c.start(); // connects IPC — returns ()
                        thread::sleep(Duration::from_millis(300));
                        current_client_id = new_settings.client_id.clone();
                        client = Some(c);
                    }
                }

                settings = new_settings;

                let Some(ref mut c) = client else { continue };

                // Determine which activity to show
                let music_active = title.is_some() && settings.use_music_when_playing;

                let (details, state) = if music_active {
                    (
                        title.as_deref().unwrap_or("").to_string(),
                        format!("by {}", artist.as_deref().unwrap_or("")),
                    )
                } else {
                    (
                        settings.custom_details.clone(),
                        settings.custom_state.clone(),
                    )
                };

                let large_key = if !settings.large_image_key.is_empty() {
                    &settings.large_image_key
                } else {
                    "osmv_logo" // fallback default
                };

                let _ = c.set_activity(|a| {
                    let a = a.details(&details).state(&state);
                    let a = a.assets(|assets| {
                        let assets = assets
                            .large_image(large_key)
                            .large_text(&settings.large_image_text);
                        if !settings.small_image_key.is_empty() {
                            assets
                                .small_image(&settings.small_image_key)
                                .small_text(&settings.small_image_text)
                        } else {
                            assets
                        }
                    });
                    a
                });
            }
        }
    }

    // Clean up on shutdown
    if let Some(ref mut c) = client {
        let _ = c.clear_activity();
    }
}
