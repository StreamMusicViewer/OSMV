// src-rust/utils.rs
// JSON writing, settings persistence, and thumbnail URL fetching

use std::path::PathBuf;

use base64::Engine;
use serde::{Deserialize, Serialize};

// ── Settings ──────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct DiscordSettings {
    /// Enable Discord Rich Presence
    #[serde(default)]
    pub enabled: bool,
    /// Use current playing music as the RPC activity (when playing)
    #[serde(default = "default_true")]
    pub use_music_when_playing: bool,
    /// Your Discord Application Client ID
    #[serde(default)]
    pub client_id: String,
    /// Default activity details (line 1, shown when no music or use_music is off)
    #[serde(default)]
    pub custom_details: String,
    /// Default activity state (line 2)
    #[serde(default)]
    pub custom_state: String,
    /// Large image key / placeholder (from Discord app Art Assets)
    /// Used when there is no HTTPS art URL available
    #[serde(default)]
    pub large_image_key: String,
    /// Large image tooltip text
    #[serde(default)]
    pub large_image_text: String,
    /// Small image key shown in bottom-right when music is PLAYING
    #[serde(default = "default_key_playing")]
    pub status_key_playing: String,
    /// Small image key shown in bottom-right when music is PAUSED
    #[serde(default = "default_key_paused")]
    pub status_key_paused: String,
    /// Small image key shown in bottom-right when music is STOPPED
    #[serde(default = "default_key_stopped")]
    pub status_key_stopped: String,
    /// Small image key used in custom / idle mode
    #[serde(default)]
    pub small_image_key: String,
    /// Small image tooltip text (custom/idle mode)
    #[serde(default)]
    pub small_image_text: String,
}

fn default_true() -> bool { true }
fn default_key_playing() -> String { "playing".into() }
fn default_key_paused()  -> String { "paused".into() }
fn default_key_stopped() -> String { "stopped".into() }

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Settings {
    #[serde(rename = "dynamicColor", default)]
    pub dynamic_color: bool,
    #[serde(rename = "audioVisualizer", default)]
    pub audio_visualizer: bool,
    #[serde(default)]
    pub discord: DiscordSettings,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            dynamic_color: false,
            audio_visualizer: false,
            discord: DiscordSettings {
                use_music_when_playing: true,
                ..Default::default()
            },
        }
    }
}

/// Path of the executable's directory.
pub fn exe_dir() -> PathBuf {
    std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.to_path_buf()))
        .unwrap_or_else(|| PathBuf::from("."))
}

pub fn json_output_path() -> PathBuf {
    exe_dir().join("current_song.json")
}

pub fn settings_path() -> PathBuf {
    exe_dir().join("settings.json")
}

// ── Settings I/O ─────────────────────────────────────────────────────────────

pub fn load_settings() -> Settings {
    let path = settings_path();
    std::fs::read_to_string(&path)
        .ok()
        .and_then(|s| serde_json::from_str(&s).ok())
        .unwrap_or_default()
}

pub fn save_settings(settings: &Settings) {
    let path = settings_path();
    if let Ok(json) = serde_json::to_string_pretty(settings) {
        let _ = std::fs::write(path, json);
    }
}

// ── JSON output ───────────────────────────────────────────────────────────────

#[derive(Serialize)]
struct SongJson<'a> {
    title: &'a str,
    artist: &'a str,
    album: &'a str,
    thumbnail: &'a str,
    status: &'a str,
    #[serde(rename = "dynamicColor")]
    dynamic_color: bool,
    #[serde(rename = "audioVisualizer")]
    audio_visualizer: bool,
    timestamp: String,
}

pub fn write_json(
    title: &str,
    artist: &str,
    album: &str,
    status: &str,
    thumbnail_b64: &str,
    dynamic_color: bool,
    audio_visualizer: bool,
) {
    let timestamp = {
        use std::time::{SystemTime, UNIX_EPOCH};
        let secs = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_secs())
            .unwrap_or(0);
        format_unix_as_iso8601(secs)
    };

    let obj = SongJson { title, artist, album, thumbnail: thumbnail_b64, status, dynamic_color, audio_visualizer, timestamp };
    if let Ok(json) = serde_json::to_string_pretty(&obj) {
        let _ = std::fs::write(json_output_path(), json);
    }
}

pub fn write_null_json() {
    let _ = std::fs::write(json_output_path(), "null\n");
}

// ── Thumbnail fetching ────────────────────────────────────────────────────────

pub fn load_thumbnail_as_base64(url: &str) -> String {
    if url.is_empty() { return String::new(); }

    if let Some(path) = url.strip_prefix("file://") {
        let decoded = percent_decode(path);
        return std::fs::read(&decoded)
            .map(|b| base64::engine::general_purpose::STANDARD.encode(&b))
            .unwrap_or_default();
    }

    match ureq::get(url).set("User-Agent", "OSMV/2.0").call() {
        Ok(resp) => {
            let mut buf = Vec::new();
            use std::io::Read;
            if resp.into_reader().read_to_end(&mut buf).is_ok() {
                return base64::engine::general_purpose::STANDARD.encode(&buf);
            }
            String::new()
        }
        Err(_) => String::new(),
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

fn percent_decode(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let mut chars = s.chars().peekable();
    while let Some(c) = chars.next() {
        if c == '%' {
            let h1 = chars.next().unwrap_or('0');
            let h2 = chars.next().unwrap_or('0');
            if let Ok(b) = u8::from_str_radix(&format!("{h1}{h2}"), 16) {
                out.push(b as char);
            }
        } else {
            out.push(c);
        }
    }
    out
}

fn format_unix_as_iso8601(secs: u64) -> String {
    let (y, m, d, h, mi, s) = unix_to_parts(secs);
    format!("{y:04}-{m:02}-{d:02}T{h:02}:{mi:02}:{s:02}Z")
}

fn unix_to_parts(secs: u64) -> (u32, u32, u32, u32, u32, u32) {
    let s = (secs % 60) as u32;
    let mins = secs / 60;
    let mi = (mins % 60) as u32;
    let hours = mins / 60;
    let h = (hours % 24) as u32;
    let days = (hours / 24) as u32;
    let z = days + 719468;
    let era = z / 146097;
    let doe = z - era * 146097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y = (yoe + era * 400) as u32;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = (doy - (153 * mp + 2) / 5 + 1) as u32;
    let m = if mp < 10 { mp + 3 } else { mp - 9 } as u32;
    let y = if m <= 2 { y + 1 } else { y };
    (y, m, d, h, mi, s)
}
