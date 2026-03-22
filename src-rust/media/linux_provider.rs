// src-rust/media/linux_provider.rs
// Linux implementation using the `mpris` crate over D-Bus.
// Requires playerctld / any MPRIS-compatible player to be running.

use super::{MediaProvider, SongInfo};

pub struct LinuxMediaProvider {
    player_finder: mpris::PlayerFinder,
}

impl LinuxMediaProvider {
    pub fn new() -> Self {
        Self {
            player_finder: mpris::PlayerFinder::new().expect("Could not connect to D-Bus"),
        }
    }
}

impl MediaProvider for LinuxMediaProvider {
    fn current_song(&mut self) -> SongInfo {
        let mut info = SongInfo {
            status: "closed".into(),
            ..Default::default()
        };

        // Find the active player
        let players = match self.player_finder.find_all() {
            Ok(p) if !p.is_empty() => p,
            _ => return info,
        };

        // Prefer the first playing player, otherwise take the first one
        let player = players
            .into_iter()
            .find(|p| p.get_playback_status().ok() == Some(mpris::PlaybackStatus::Playing))
            .or_else(|| {
                self.player_finder.find_all().ok().and_then(|mut v| {
                    if v.is_empty() { None } else { Some(v.remove(0)) }
                })
            });

        let player = match player {
            Some(p) => p,
            None => return info,
        };

        // ── Playback status ──────────────────────────────────────────────────
        match player.get_playback_status() {
            Ok(mpris::PlaybackStatus::Playing) => {
                info.status = "playing".into();
                info.is_playing = true;
            }
            Ok(mpris::PlaybackStatus::Paused) => info.status = "paused".into(),
            Ok(mpris::PlaybackStatus::Stopped) => info.status = "stopped".into(),
            _ => info.status = "closed".into(),
        }

        // ── Metadata ─────────────────────────────────────────────────────────
        if let Ok(metadata) = player.get_metadata() {
            info.title = metadata
                .title()
                .unwrap_or("Unknown Title")
                .to_string();
            info.artist = metadata
                .artists()
                .and_then(|a| a.first().map(|s| s.to_string()))
                .unwrap_or_else(|| "Unknown Artist".into());
            info.album = metadata
                .album_name()
                .unwrap_or_default()
                .to_string();
            info.art_url = metadata
                .art_url()
                .unwrap_or_default()
                .to_string();
        }

        info
    }
}
