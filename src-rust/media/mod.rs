// src-rust/media/mod.rs
// Platform-agnostic media info trait

#[derive(Debug, Clone, Default)]
pub struct SongInfo {
    pub title: String,
    pub artist: String,
    pub album: String,
    /// file:// or https:// art URL (Linux only, raw from MPRIS)
    pub art_url: String,
    /// Base64-encoded thumbnail image data
    pub thumbnail_b64: String,
    /// "playing" | "paused" | "stopped" | "closed"
    pub status: String,
    pub is_playing: bool,
}

pub trait MediaProvider: Send {
    fn current_song(&mut self) -> SongInfo;
}

/// Factory: returns the right implementation for the current platform.
pub fn create_provider() -> Box<dyn MediaProvider> {
    #[cfg(target_os = "windows")]
    {
        Box::new(super::media::windows_provider::WindowsMediaProvider::new())
    }
    #[cfg(target_os = "linux")]
    {
        Box::new(super::media::linux_provider::LinuxMediaProvider::new())
    }
    #[cfg(not(any(target_os = "windows", target_os = "linux")))]
    {
        compile_error!("Unsupported platform — only Windows and Linux are supported.");
    }
}

#[cfg(target_os = "windows")]
pub mod windows_provider;

#[cfg(target_os = "linux")]
pub mod linux_provider;
