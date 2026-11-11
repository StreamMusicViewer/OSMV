#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace osmv {

struct DiscordSettings {
    bool enabled = false;
    bool use_music_when_playing = true;
    std::string client_id;
    std::string custom_details;
    std::string custom_state;
    std::string large_image_key;
    std::string large_image_text;
    std::string status_key_playing = "playing";
    std::string status_key_paused = "paused";
    std::string status_key_stopped = "stopped";
    std::string small_image_key;
    std::string small_image_text;
};

struct TimeSettings {
    bool enabled = false;
    std::string format = "$h:$m:$s $tt";
    bool use_ampm = false;
};

struct DateSettings {
    bool enabled = false;
    std::string date_format = "$dddd dd MMMM yyyy"; // $dd-MM-yy | $dd-MM-yyyy | $dd MMMM yyyy | $dddd dd MMMM yyyy
    std::string output_format = "$date";
};

struct CountdownSettings {
    bool enabled = false;
    int target_year = 2026;
    int target_month = 12;
    int target_day = 31;
    int target_hour = 23;
    int target_minute = 59;
    int target_second = 59;
    std::string format = "$d d $h:$m:$s";
    std::string done_text = "00:00:00";
    bool pad_zeros = true;
};

struct ChronoDownSettings {
    bool enabled = false;
    int initial_hours = 0;
    int initial_minutes = 5;
    int initial_seconds = 0;
    std::string format = "$h:$m:$s";
    std::string done_text = "00:00:00";
    bool pad_zeros = true;
};

struct ChronoUpSettings {
    bool enabled = false;
    int start_hours = 0;
    int start_minutes = 0;
    int start_seconds = 0;
    std::string format = "$h:$m:$s";
    bool pad_zeros = true;
};

struct SystemInfoSettings {
    bool enabled = false;
    int interval_sec = 1;
    std::string format = "CPU: $CPU_USAGE | RAM: $RAM_%_IN_USE | Down: $DOWNLOAD";
};

struct TextlineSettings {
    bool enabled = false;
    int interval_sec = 10;
    bool random_order = false;
    bool skip_empty = true;
    std::string filename = "TextlineChanger.txt";
    std::vector<std::string> lines = {
        "Welcome to the stream!",
        "Follow & enjoy the music!"
    };
};

struct DynamicFileRule {
    bool enabled = true;
    std::string filename = "DynamicOutput.txt";
    std::string url;
    std::string type = "text"; // "text" | "image"
    int interval_sec = 30;
    std::string headers; // optional newline-separated "Key: Value"
};

struct DynamicFilesSettings {
    bool enabled = false;
    std::vector<DynamicFileRule> rules;
};

struct TwitchCustomGame {
    std::string id;
    std::string name;
};

struct TwitchSettings {
    bool enabled = false;
    std::string client_id;
    std::string oauth_token;
    std::string channel;
    int interval_sec = 60;
    std::string viewers_format = "Viewers: $viewers";
    std::string offline_text = "Offline";
    std::vector<TwitchCustomGame> custom_games;
};

struct NowPlayingExportSettings {
    bool export_text_files = false;      // TrackArtist.txt, TrackSong.txt, TrackAlbum.txt
    bool export_formatted_file = false;  // PlayingNow.txt
    std::string custom_format = "$artist - '$song'";
    bool export_snaz_json = false;       // track-info.json
    bool clear_when_inactive = false;
    std::string fallback_text;
};

struct ObsWidgetSettings {
    std::string preset = "classic"; // "classic" | "glass" | "neon" | "compact" | "vinyl" | "minimal" | "banner" | "custom"

    // Layout & geometry
    std::string position = "bottom-right"; // "bottom-right" | "bottom-left" | "top-right" | "top-left" | "center"
    std::string layout = "horizontal";     // "horizontal" | "horizontal-right" | "vertical" | "compact"
    int width_px = 450;
    int padding_px = 20;
    int gap_px = 16;
    int border_radius_px = 16;
    int margin_px = 20;

    // Album art
    bool show_album_art = true;
    int art_size_px = 80;
    int art_radius_px = 8;
    bool spin_artwork = false;

    // Content & formatting
    bool show_title = true;
    bool show_artist = true;
    bool show_album = false;
    bool show_status_badge = false;
    std::string title_format = "$song";
    std::string artist_format = "$artist";
    std::string album_format = "$album";
    std::string badge_format = "$status_upper";
    std::string idle_text = "Nothing currently playing";
    bool marquee_scroll = true;
    float scroll_speed = 1.0f;

    // Colors, glass, borders & glow
    std::string bg_color = "#0F0F0FE0";
    bool use_bg_gradient = false;
    std::string bg_color_2 = "#1D1B38E0";
    int backdrop_blur_px = 0;
    std::string border_color = "#FFFFFF1A";
    int border_width_px = 1;
    std::string accent_color = "#7C5CFFFF";
    std::string title_color = "#FFFFFFFF";
    std::string artist_color = "#B3B3B3FF";
    std::string album_color = "#8A9BBFFF";
    float shadow_intensity = 0.45f;
    bool glow_enabled = false;
    std::string dynamic_color_mode = "card"; // "card" | "accent" | "subtle"

    // Typography
    std::string font_family = "Segoe UI";
    int title_size_px = 18;
    int artist_size_px = 14;
    int album_size_px = 12;
    int title_weight = 600;
    std::string text_align = "left"; // "left" | "center" | "right"
    bool text_shadow = true;
    bool load_google_font = false;

    // Audio visualizer
    std::string viz_position = "top";   // "top" | "bottom" | "background"
    std::string viz_style = "bars";     // "bars" | "mirrored" | "dots" | "wave" | "blocks"
    int viz_bar_count = 20;
    int viz_height_px = 56;
    int viz_gap_px = 4;
    std::string viz_color_mode = "album"; // "album" | "custom" | "rainbow"
    std::string viz_color_top = "#9B7BFFFF";
    std::string viz_color_bottom = "#4C33B3FF";
    float viz_speed = 1.0f;

    // Idle & transitions
    bool hide_when_idle = true;
    int hide_delay_sec = 10;
    std::string transition_style = "fade"; // "fade" | "slide-up" | "slide-left" | "zoom" | "none"

    // Custom CSS
    bool custom_css_enabled = false;
    bool custom_css_raw_override = false;
    std::string custom_css;
};

struct ObsCustomPreset {
    std::string name;
    bool dynamic_color = false;
    bool audio_visualizer = false;
    ObsWidgetSettings widget;
};

struct GeneralSettings {
    std::string language = "fr"; // "fr" | "en"
    std::string theme = "dark";  // "dark" | "light"
    std::string accent = "blurple"; // "blurple" | "emerald" | "cyan" | "rose" | "amber"
    std::string output_dir;      // empty -> shared/dynamic_text
    bool open_gui_on_startup = true;
    bool autostart = false;
};

struct AppSettings {
    bool now_playing_enabled = true;
    bool dynamic_color = false;
    bool audio_visualizer = false;
    NowPlayingExportSettings now_playing;
    ObsWidgetSettings obs_widget;
    std::vector<ObsCustomPreset> obs_custom_presets;
    DiscordSettings discord;
    TimeSettings time;
    DateSettings date;
    CountdownSettings countdown;
    ChronoDownSettings chrono_down;
    ChronoUpSettings chrono_up;
    SystemInfoSettings system_info;
    TextlineSettings textline;
    DynamicFilesSettings dynamic_files;
    TwitchSettings twitch;
    GeneralSettings general;
};

// ── Live runtime state shared from Daemon -> GUI via IPC ─────────────────────

struct SongInfo {
    std::string title;
    std::string artist;
    std::string album;
    std::string art_url;
    std::string thumbnail_b64; // used internally by daemon / JSON
    std::string status = "closed"; // "playing" | "paused" | "stopped" | "closed"
    bool is_playing = false;
};

struct SystemMetrics {
    double cpu_percent = 0.0;
    uint64_t ram_avail_mb = 0;
    uint64_t ram_total_mb = 0;
    double ram_in_use_percent = 0.0;
    double ram_avail_percent = 0.0;
    uint32_t process_count = 0;
    double upload_bytes_per_sec = 0.0;
    double download_bytes_per_sec = 0.0;
};

struct DynamicRuleStatus {
    std::string filename;
    std::string status = "Idle";
    std::string preview;
    bool ok = true;
};

struct TwitchStatus {
    bool connected = false;
    bool is_live = false;
    int viewers = 0;
    std::string broadcaster_id;
    std::string current_title;
    std::string current_game;
    std::string status_text = "Disabled";
};

struct DaemonStateSnapshot {
    // Song
    std::string title;
    std::string artist;
    std::string album;
    std::string player_name;
    std::string status = "closed";
    std::string thumbnail_file_path; // temp file written by daemon only when song changes
    std::string playing_now_preview;

    // Time & Date
    std::string time_pc_preview;
    std::string time_live_preview;
    std::string date_live_preview;

    // Timers
    std::string countdown_preview;
    bool countdown_reached = false;
    std::string chrono_down_preview;
    bool chrono_down_running = false;
    int64_t chrono_down_remaining_ms = 0;
    std::string chrono_up_preview;
    bool chrono_up_running = false;
    int64_t chrono_up_elapsed_ms = 0;

    // System Info
    SystemMetrics sys_metrics;
    std::string sysinfo_preview;

    // Textline & Dynamic Files
    std::string textline_preview;
    int textline_index = 0;
    std::vector<DynamicRuleStatus> dynamic_statuses;

    // Discord
    std::string discord_status = "disabled"; // "disabled" | "connecting" | "connected" | "error" | "invalid_id"

    // Twitch
    TwitchStatus twitch;

    // Memory telemetry (KB)
    uint64_t daemon_ram_kb = 0;
};

} // namespace osmv

