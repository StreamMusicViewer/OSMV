// src-rust/media/windows_provider.rs
// Windows implementation using WinRT SMTC

use super::{MediaProvider, SongInfo};
use base64::Engine as _;
use windows::Media::Control::{
    GlobalSystemMediaTransportControlsSessionManager,
    GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus,
};
use windows::Storage::Streams::DataReader;

pub struct WindowsMediaProvider {
    manager: Option<GlobalSystemMediaTransportControlsSessionManager>,
}

impl WindowsMediaProvider {
    pub fn new() -> Self {
        let manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync()
            .and_then(|op| op.get())
            .ok();
        Self { manager }
    }

    fn stream_ref_to_base64(
        stream_ref: &windows::Storage::Streams::IRandomAccessStreamReference,
    ) -> String {
        let stream = match stream_ref.OpenReadAsync().and_then(|op| op.get()) {
            Ok(s) => s,
            Err(_) => return String::new(),
        };
        let size = match stream.Size() {
            Ok(s) if s > 0 => s,
            _ => return String::new(),
        };
        let reader = match DataReader::CreateDataReader(&stream) {
            Ok(r) => r,
            Err(_) => return String::new(),
        };
        if reader.LoadAsync(size as u32).and_then(|op| op.get()).is_err() {
            return String::new();
        }
        let mut buf = vec![0u8; size as usize];
        if reader.ReadBytes(&mut buf).is_err() {
            return String::new();
        }
        base64::engine::general_purpose::STANDARD.encode(&buf)
    }
}

impl MediaProvider for WindowsMediaProvider {
    fn current_song(&mut self) -> SongInfo {
        let mut info = SongInfo {
            status: "closed".into(),
            ..Default::default()
        };

        let manager = match &self.manager {
            Some(m) => m,
            None => return info,
        };

        let session = match manager.GetCurrentSession() {
            Ok(s) => s,
            Err(_) => return info,
        };

        // ── Playback status ──────────────────────────────────────────────────
        if let Ok(playback) = session.GetPlaybackInfo() {
            if let Ok(status) = playback.PlaybackStatus() {
                match status {
                    PlaybackStatus::Closed => info.status = "closed".into(),
                    PlaybackStatus::Opened => info.status = "opened".into(),
                    PlaybackStatus::Changing => info.status = "changing".into(),
                    PlaybackStatus::Stopped => info.status = "stopped".into(),
                    PlaybackStatus::Playing => {
                        info.status = "playing".into();
                        info.is_playing = true;
                    }
                    PlaybackStatus::Paused => info.status = "paused".into(),
                    _ => info.status = "unknown".into(),
                }
            }
        }

        // ── Media properties ─────────────────────────────────────────────────
        if let Ok(props) = session.TryGetMediaPropertiesAsync().and_then(|op| op.get()) {
            info.title = props
                .Title()
                .map(|s| s.to_string())
                .unwrap_or_else(|_| "Unknown Title".into());
            info.artist = props
                .Artist()
                .map(|s| s.to_string())
                .unwrap_or_else(|_| "Unknown Artist".into());
            info.album = props
                .AlbumTitle()
                .map(|s| s.to_string())
                .unwrap_or_default();

            if info.title.is_empty() { info.title = "Unknown Title".into(); }
            if info.artist.is_empty() { info.artist = "Unknown Artist".into(); }

            // Thumbnail
            if let Ok(thumb) = props.Thumbnail() {
                info.thumbnail_b64 = Self::stream_ref_to_base64(&thumb);
            }
        }

        info
    }
}
