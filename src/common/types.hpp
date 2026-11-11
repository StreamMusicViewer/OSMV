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

