#include "settings.hpp"
#include "formatter.hpp"
#include "os_utils.hpp"

#include <nlohmann/json.hpp>
#include <fstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace osmv::cfg {

using json = nlohmann::json;

namespace {
template <typename T>
void read_opt(const json& j, const char* key, T& dst) {
    if (j.contains(key) && !j[key].is_null()) {
        try {
            dst = j[key].get<T>();
        } catch (...) {
        }
    }
}

std::string sanitize_filename(std::string name) {
    if (name.empty()) return "output.txt";
    // Strip directory traversal components
    for (char& c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return name;
}
} // namespace

std::filesystem::path shared_dir() {
    auto exe = os::exe_dir();
    std::error_code ec;
    auto probe = exe;
    for (int depth = 0; depth < 4; ++depth) {
        auto candidate = probe / "shared";
        if (std::filesystem::is_directory(candidate, ec)) {
            return candidate;
        }
        if (!probe.has_parent_path() || probe.parent_path() == probe) break;
        probe = probe.parent_path();
    }
    // Check cwd as fallback
    auto cwd_candidate = std::filesystem::current_path(ec) / "shared";
    if (std::filesystem::is_directory(cwd_candidate, ec)) {
        return cwd_candidate;
    }
    return exe;
}

std::filesystem::path settings_path() {
    return shared_dir() / "settings.json";
}

std::filesystem::path current_song_path() {
    return shared_dir() / "current_song.json";
}

std::filesystem::path obs_widget_html_path() {
    return shared_dir() / "index.html";
}

std::filesystem::path games_custom_json_path() {
    return shared_dir() / "games-custom.json";
}

std::filesystem::path dynamic_text_dir(const AppSettings& s) {
    if (!s.general.output_dir.empty()) {
        return os::path_from_utf8(s.general.output_dir);
    }
    return shared_dir() / "dynamic_text";
}

bool write_file_bytes(const std::filesystem::path& path, const void* data, size_t size) {
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    auto tmp_path = path;
    tmp_path += ".tmp";

    {
        std::ofstream f(tmp_path, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        if (size > 0 && data) {
            f.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        }
        f.flush();
        if (!f) return false;
    }

#ifdef _WIN32
    if (MoveFileExW(tmp_path.wstring().c_str(), path.wstring().c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return true;
    }
    // Fallback direct write if target is temporarily held open without FILE_SHARE_DELETE
    std::filesystem::remove(tmp_path, ec);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    if (size > 0 && data) {
        f.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    }
    return static_cast<bool>(f);
#else
    std::filesystem::rename(tmp_path, path, ec);
    if (!ec) return true;
    std::filesystem::remove(tmp_path, ec);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    if (size > 0 && data) {
        f.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    }
    return static_cast<bool>(f);
#endif
}

bool write_file_text(const std::filesystem::path& path, const std::string& content) {
    return write_file_bytes(path, content.data(), content.size());
}

void CachedTextWriter::write_if_changed(const std::filesystem::path& path, const std::string& content) {
    std::string key = os::path_to_utf8(path);
    auto it = cache_.find(key);
    if (it != cache_.end() && it->second == content) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            return;
        }
    }
    if (write_file_text(path, content)) {
        cache_[std::move(key)] = content;
    }
}

void CachedTextWriter::remove_if_exists(const std::filesystem::path& path) {
    std::string key = os::path_to_utf8(path);
    cache_.erase(key);
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        std::filesystem::remove(path, ec);
    }
}

void CachedTextWriter::clear_cache() {
    cache_.clear();
}

void write_current_song_json(const std::string& title,
                             const std::string& artist,
                             const std::string& album,
                             const std::string& status,
                             const std::string& thumbnail_b64,
                             bool dynamic_color,
                             bool audio_visualizer) {
    json j = {
        {"title", title},
        {"artist", artist},
        {"album", album},
        {"thumbnail", thumbnail_b64},
        {"status", status},
        {"dynamicColor", dynamic_color},
        {"audioVisualizer", audio_visualizer},
        {"timestamp", fmt::format_iso8601_utc()}
    };
    write_file_text(current_song_path(), j.dump(2));
}

void write_null_song_json() {
    write_file_text(current_song_path(), "null\n");
}

void write_snaz_track_info_json(const std::filesystem::path& dir,
                                 const std::string& player_name,
                                 const std::string& artist,
                                 const std::string& song,
                                 const std::string& album,
                                 const std::string& status) {
    json j = {
        {"mediaplayer", player_name},
        {"artist", artist},
        {"song", song},
        {"album", album},
        {"status", status}
    };
    write_file_text(dir / "track-info.json", j.dump(2));
}

std::string settings_to_json_string(const AppSettings& s, bool pretty) {
    json j;
    j["nowPlayingEnabled"] = s.now_playing_enabled;
    j["dynamicColor"] = s.dynamic_color;
    j["audioVisualizer"] = s.audio_visualizer;

    j["nowPlaying"] = {
        {"exportTextFiles", s.now_playing.export_text_files},
        {"exportFormattedFile", s.now_playing.export_formatted_file},
        {"customFormat", s.now_playing.custom_format},
        {"exportSnazJson", s.now_playing.export_snaz_json},
        {"clearWhenInactive", s.now_playing.clear_when_inactive},
        {"fallbackText", s.now_playing.fallback_text}
    };

    j["discord"] = {
        {"enabled", s.discord.enabled},
        {"use_music_when_playing", s.discord.use_music_when_playing},
        {"client_id", s.discord.client_id},
        {"custom_details", s.discord.custom_details},
        {"custom_state", s.discord.custom_state},
        {"large_image_key", s.discord.large_image_key},
        {"large_image_text", s.discord.large_image_text},
        {"status_key_playing", s.discord.status_key_playing},
        {"status_key_paused", s.discord.status_key_paused},
        {"status_key_stopped", s.discord.status_key_stopped},
        {"small_image_key", s.discord.small_image_key},
        {"small_image_text", s.discord.small_image_text}
    };

    j["time"] = {
        {"enabled", s.time.enabled},
        {"format", s.time.format},
        {"use_ampm", s.time.use_ampm}
    };

    j["date"] = {
        {"enabled", s.date.enabled},
        {"dateFormat", s.date.date_format},
        {"outputFormat", s.date.output_format}
    };

    j["countdown"] = {
        {"enabled", s.countdown.enabled},
        {"targetYear", s.countdown.target_year},
        {"targetMonth", s.countdown.target_month},
        {"targetDay", s.countdown.target_day},
        {"targetHour", s.countdown.target_hour},
        {"targetMinute", s.countdown.target_minute},
        {"targetSecond", s.countdown.target_second},
        {"format", s.countdown.format},
        {"doneText", s.countdown.done_text},
        {"padZeros", s.countdown.pad_zeros}
    };

    j["chronoDown"] = {
        {"enabled", s.chrono_down.enabled},
        {"initialHours", s.chrono_down.initial_hours},
        {"initialMinutes", s.chrono_down.initial_minutes},
        {"initialSeconds", s.chrono_down.initial_seconds},
        {"format", s.chrono_down.format},
        {"doneText", s.chrono_down.done_text},
        {"padZeros", s.chrono_down.pad_zeros}
    };

    j["chronoUp"] = {
        {"enabled", s.chrono_up.enabled},
        {"startHours", s.chrono_up.start_hours},
        {"startMinutes", s.chrono_up.start_minutes},
        {"startSeconds", s.chrono_up.start_seconds},
        {"format", s.chrono_up.format},
        {"padZeros", s.chrono_up.pad_zeros}
    };

    j["systemInfo"] = {
        {"enabled", s.system_info.enabled},
        {"intervalSec", s.system_info.interval_sec},
        {"format", s.system_info.format}
    };

    j["textlineChanger"] = {
        {"enabled", s.textline.enabled},
        {"intervalSec", s.textline.interval_sec},
        {"randomOrder", s.textline.random_order},
        {"skipEmpty", s.textline.skip_empty},
        {"filename", s.textline.filename},
        {"lines", s.textline.lines}
    };

    json rules_json = json::array();
    for (const auto& r : s.dynamic_files.rules) {
        rules_json.push_back({
            {"enabled", r.enabled},
            {"filename", r.filename},
            {"url", r.url},
            {"type", r.type},
            {"intervalSec", r.interval_sec},
            {"headers", r.headers}
        });
    }
    j["dynamicFiles"] = {
        {"enabled", s.dynamic_files.enabled},
        {"rules", std::move(rules_json)}
    };

    json games_json = json::array();
    for (const auto& g : s.twitch.custom_games) {
        games_json.push_back({{"id", g.id}, {"name", g.name}});
    }
    j["twitch"] = {
        {"enabled", s.twitch.enabled},
        {"clientId", s.twitch.client_id},
        {"oauthToken", s.twitch.oauth_token},
        {"channel", s.twitch.channel},
        {"intervalSec", s.twitch.interval_sec},
        {"viewersFormat", s.twitch.viewers_format},
        {"offlineText", s.twitch.offline_text},
        {"customGames", std::move(games_json)}
    };

    j["general"] = {
        {"language", s.general.language},
        {"theme", s.general.theme},
        {"accent", s.general.accent},
        {"outputDir", s.general.output_dir},
        {"openGuiOnStartup", s.general.open_gui_on_startup},
        {"autostart", s.general.autostart}
    };

    return pretty ? j.dump(2) : j.dump();
}

bool settings_from_json_string(const std::string& json_str, AppSettings& out) {
    try {
        json j = json::parse(json_str);
        if (!j.is_object()) return false;

        out = AppSettings{};
        read_opt(j, "nowPlayingEnabled", out.now_playing_enabled);
        read_opt(j, "dynamicColor", out.dynamic_color);
        read_opt(j, "audioVisualizer", out.audio_visualizer);

        if (j.contains("nowPlaying") && j["nowPlaying"].is_object()) {
            const auto& np = j["nowPlaying"];
            read_opt(np, "exportTextFiles", out.now_playing.export_text_files);
            read_opt(np, "exportFormattedFile", out.now_playing.export_formatted_file);
            read_opt(np, "customFormat", out.now_playing.custom_format);
            read_opt(np, "exportSnazJson", out.now_playing.export_snaz_json);
            read_opt(np, "clearWhenInactive", out.now_playing.clear_when_inactive);
            read_opt(np, "fallbackText", out.now_playing.fallback_text);
        }

        if (j.contains("discord") && j["discord"].is_object()) {
            const auto& dc = j["discord"];
            read_opt(dc, "enabled", out.discord.enabled);
            read_opt(dc, "use_music_when_playing", out.discord.use_music_when_playing);
            read_opt(dc, "client_id", out.discord.client_id);
            read_opt(dc, "custom_details", out.discord.custom_details);
            read_opt(dc, "custom_state", out.discord.custom_state);
            read_opt(dc, "large_image_key", out.discord.large_image_key);
            read_opt(dc, "large_image_text", out.discord.large_image_text);
            read_opt(dc, "status_key_playing", out.discord.status_key_playing);
            read_opt(dc, "status_key_paused", out.discord.status_key_paused);
            read_opt(dc, "status_key_stopped", out.discord.status_key_stopped);
            read_opt(dc, "small_image_key", out.discord.small_image_key);
            read_opt(dc, "small_image_text", out.discord.small_image_text);
        }

        if (j.contains("time") && j["time"].is_object()) {
            const auto& t = j["time"];
            read_opt(t, "enabled", out.time.enabled);
            read_opt(t, "format", out.time.format);
            read_opt(t, "use_ampm", out.time.use_ampm);
        }

        if (j.contains("date") && j["date"].is_object()) {
            const auto& d = j["date"];
            read_opt(d, "enabled", out.date.enabled);
            read_opt(d, "dateFormat", out.date.date_format);
            read_opt(d, "outputFormat", out.date.output_format);
        }

        if (j.contains("countdown") && j["countdown"].is_object()) {
            const auto& c = j["countdown"];
            read_opt(c, "enabled", out.countdown.enabled);
            read_opt(c, "targetYear", out.countdown.target_year);
            read_opt(c, "targetMonth", out.countdown.target_month);
            read_opt(c, "targetDay", out.countdown.target_day);
            read_opt(c, "targetHour", out.countdown.target_hour);
            read_opt(c, "targetMinute", out.countdown.target_minute);
            read_opt(c, "targetSecond", out.countdown.target_second);
            read_opt(c, "format", out.countdown.format);
            read_opt(c, "doneText", out.countdown.done_text);
            read_opt(c, "padZeros", out.countdown.pad_zeros);
        }

        if (j.contains("chronoDown") && j["chronoDown"].is_object()) {
            const auto& cd = j["chronoDown"];
            read_opt(cd, "enabled", out.chrono_down.enabled);
            read_opt(cd, "initialHours", out.chrono_down.initial_hours);
            read_opt(cd, "initialMinutes", out.chrono_down.initial_minutes);
            read_opt(cd, "initialSeconds", out.chrono_down.initial_seconds);
            read_opt(cd, "format", out.chrono_down.format);
            read_opt(cd, "doneText", out.chrono_down.done_text);
            read_opt(cd, "padZeros", out.chrono_down.pad_zeros);
        }

        if (j.contains("chronoUp") && j["chronoUp"].is_object()) {
            const auto& cu = j["chronoUp"];
            read_opt(cu, "enabled", out.chrono_up.enabled);
            read_opt(cu, "startHours", out.chrono_up.start_hours);
            read_opt(cu, "startMinutes", out.chrono_up.start_minutes);
            read_opt(cu, "startSeconds", out.chrono_up.start_seconds);
            read_opt(cu, "format", out.chrono_up.format);
            read_opt(cu, "padZeros", out.chrono_up.pad_zeros);
        }

        if (j.contains("systemInfo") && j["systemInfo"].is_object()) {
            const auto& si = j["systemInfo"];
            read_opt(si, "enabled", out.system_info.enabled);
            read_opt(si, "intervalSec", out.system_info.interval_sec);
            read_opt(si, "format", out.system_info.format);
            out.system_info.interval_sec = std::clamp(out.system_info.interval_sec, 1, 60);
        }

        if (j.contains("textlineChanger") && j["textlineChanger"].is_object()) {
            const auto& tl = j["textlineChanger"];
            read_opt(tl, "enabled", out.textline.enabled);
            read_opt(tl, "intervalSec", out.textline.interval_sec);
            read_opt(tl, "randomOrder", out.textline.random_order);
            read_opt(tl, "skipEmpty", out.textline.skip_empty);
            read_opt(tl, "filename", out.textline.filename);
            out.textline.filename = sanitize_filename(out.textline.filename);
            out.textline.interval_sec = std::clamp(out.textline.interval_sec, 1, 3600);
            if (tl.contains("lines") && tl["lines"].is_array()) {
                out.textline.lines.clear();
                for (const auto& item : tl["lines"]) {
                    if (item.is_string()) {
                        out.textline.lines.push_back(item.get<std::string>());
                    }
                }
            }
        }

        if (j.contains("dynamicFiles") && j["dynamicFiles"].is_object()) {
            const auto& df = j["dynamicFiles"];
            read_opt(df, "enabled", out.dynamic_files.enabled);
            if (df.contains("rules") && df["rules"].is_array()) {
                out.dynamic_files.rules.clear();
                for (const auto& rj : df["rules"]) {
                    if (!rj.is_object()) continue;
                    DynamicFileRule r;
                    read_opt(rj, "enabled", r.enabled);
                    read_opt(rj, "filename", r.filename);
                    read_opt(rj, "url", r.url);
                    read_opt(rj, "type", r.type);
                    read_opt(rj, "intervalSec", r.interval_sec);
                    read_opt(rj, "headers", r.headers);
                    r.filename = sanitize_filename(r.filename);
                    r.interval_sec = std::clamp(r.interval_sec, 5, 86400);
                    out.dynamic_files.rules.push_back(std::move(r));
                }
            }
        }

        if (j.contains("twitch") && j["twitch"].is_object()) {
            const auto& tw = j["twitch"];
            read_opt(tw, "enabled", out.twitch.enabled);
            read_opt(tw, "clientId", out.twitch.client_id);
            read_opt(tw, "oauthToken", out.twitch.oauth_token);
            read_opt(tw, "channel", out.twitch.channel);
            read_opt(tw, "intervalSec", out.twitch.interval_sec);
            read_opt(tw, "viewersFormat", out.twitch.viewers_format);
            read_opt(tw, "offlineText", out.twitch.offline_text);
            out.twitch.interval_sec = std::clamp(out.twitch.interval_sec, 15, 3600);
            if (tw.contains("customGames") && tw["customGames"].is_array()) {
                out.twitch.custom_games.clear();
                for (const auto& gj : tw["customGames"]) {
                    if (!gj.is_object()) continue;
                    TwitchCustomGame g;
                    read_opt(gj, "id", g.id);
                    read_opt(gj, "name", g.name);
                    if (!g.name.empty()) {
                        out.twitch.custom_games.push_back(std::move(g));
                    }
                }
            }
        }

        if (j.contains("general") && j["general"].is_object()) {
            const auto& g = j["general"];
            read_opt(g, "language", out.general.language);
            read_opt(g, "theme", out.general.theme);
            read_opt(g, "accent", out.general.accent);
            read_opt(g, "outputDir", out.general.output_dir);
            read_opt(g, "openGuiOnStartup", out.general.open_gui_on_startup);
            read_opt(g, "autostart", out.general.autostart);
        }
        return true;
    } catch (...) {
        return false;
    }
}

AppSettings load_settings() {
    AppSettings s;
    std::ifstream f(settings_path());
    if (f) {
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        settings_from_json_string(content, s);
    }

    // Import Snaz-compatible games-custom.json if custom_games is empty
    if (s.twitch.custom_games.empty()) {
        std::ifstream gf(games_custom_json_path());
        if (gf) {
            try {
                json gj = json::parse(gf);
                if (gj.contains("Games") && gj["Games"].is_object()) {
                    for (auto& [k, v] : gj["Games"].items()) {
                        if (v.is_string()) {
                            s.twitch.custom_games.push_back({k, v.get<std::string>()});
                        }
                    }
                }
            } catch (...) {
            }
        }
    }
    return s;
}

bool save_settings(const AppSettings& s) {
    return write_file_text(settings_path(), settings_to_json_string(s, true));
}

std::string snapshot_to_json_string(const DaemonStateSnapshot& snap) {
    json dyn_arr = json::array();
    for (const auto& d : snap.dynamic_statuses) {
        dyn_arr.push_back({
            {"filename", d.filename},
            {"status", d.status},
            {"preview", d.preview},
            {"ok", d.ok}
        });
    }

    json j = {
        {"title", snap.title},
        {"artist", snap.artist},
        {"album", snap.album},
        {"status", snap.status},
        {"thumbnailFilePath", snap.thumbnail_file_path},
        {"playingNowPreview", snap.playing_now_preview},
        {"timePcPreview", snap.time_pc_preview},
        {"timeLivePreview", snap.time_live_preview},
        {"dateLivePreview", snap.date_live_preview},
        {"countdownPreview", snap.countdown_preview},
        {"countdownReached", snap.countdown_reached},
        {"chronoDownPreview", snap.chrono_down_preview},
        {"chronoDownRunning", snap.chrono_down_running},
        {"chronoDownRemainingMs", snap.chrono_down_remaining_ms},
        {"chronoUpPreview", snap.chrono_up_preview},
        {"chronoUpRunning", snap.chrono_up_running},
        {"chronoUpElapsedMs", snap.chrono_up_elapsed_ms},
        {"sysMetrics", {
            {"cpu", snap.sys_metrics.cpu_percent},
            {"ramAvailMb", snap.sys_metrics.ram_avail_mb},
            {"ramTotalMb", snap.sys_metrics.ram_total_mb},
            {"ramInUsePct", snap.sys_metrics.ram_in_use_percent},
            {"ramAvailPct", snap.sys_metrics.ram_avail_percent},
            {"processes", snap.sys_metrics.process_count},
            {"uploadBps", snap.sys_metrics.upload_bytes_per_sec},
            {"downloadBps", snap.sys_metrics.download_bytes_per_sec}
        }},
        {"sysInfoPreview", snap.sysinfo_preview},
        {"textlinePreview", snap.textline_preview},
        {"textlineIndex", snap.textline_index},
        {"dynamicStatuses", std::move(dyn_arr)},
        {"discordStatus", snap.discord_status},
        {"twitch", {
            {"connected", snap.twitch.connected},
            {"isLive", snap.twitch.is_live},
            {"viewers", snap.twitch.viewers},
            {"broadcasterId", snap.twitch.broadcaster_id},
            {"currentTitle", snap.twitch.current_title},
            {"currentGame", snap.twitch.current_game},
            {"statusText", snap.twitch.status_text}
        }},
        {"daemonRamKb", snap.daemon_ram_kb}
    };
    return j.dump();
}

bool snapshot_from_json_string(const std::string& json_str, DaemonStateSnapshot& out) {
    try {
        json j = json::parse(json_str);
        if (!j.is_object()) return false;
        out = DaemonStateSnapshot{};
        read_opt(j, "title", out.title);
        read_opt(j, "artist", out.artist);
        read_opt(j, "album", out.album);
        read_opt(j, "status", out.status);
        read_opt(j, "thumbnailFilePath", out.thumbnail_file_path);
        read_opt(j, "playingNowPreview", out.playing_now_preview);
        read_opt(j, "timePcPreview", out.time_pc_preview);
        read_opt(j, "timeLivePreview", out.time_live_preview);
        read_opt(j, "dateLivePreview", out.date_live_preview);
        read_opt(j, "countdownPreview", out.countdown_preview);
        read_opt(j, "countdownReached", out.countdown_reached);
        read_opt(j, "chronoDownPreview", out.chrono_down_preview);
        read_opt(j, "chronoDownRunning", out.chrono_down_running);
        read_opt(j, "chronoDownRemainingMs", out.chrono_down_remaining_ms);
        read_opt(j, "chronoUpPreview", out.chrono_up_preview);
        read_opt(j, "chronoUpRunning", out.chrono_up_running);
        read_opt(j, "chronoUpElapsedMs", out.chrono_up_elapsed_ms);

        if (j.contains("sysMetrics") && j["sysMetrics"].is_object()) {
            const auto& m = j["sysMetrics"];
            read_opt(m, "cpu", out.sys_metrics.cpu_percent);
            read_opt(m, "ramAvailMb", out.sys_metrics.ram_avail_mb);
            read_opt(m, "ramTotalMb", out.sys_metrics.ram_total_mb);
            read_opt(m, "ramInUsePct", out.sys_metrics.ram_in_use_percent);
            read_opt(m, "ramAvailPct", out.sys_metrics.ram_avail_percent);
            read_opt(m, "processes", out.sys_metrics.process_count);
            read_opt(m, "uploadBps", out.sys_metrics.upload_bytes_per_sec);
            read_opt(m, "downloadBps", out.sys_metrics.download_bytes_per_sec);
        }
        read_opt(j, "sysInfoPreview", out.sysinfo_preview);
        read_opt(j, "textlinePreview", out.textline_preview);
        read_opt(j, "textlineIndex", out.textline_index);
        if (j.contains("dynamicStatuses") && j["dynamicStatuses"].is_array()) {
            for (const auto& dj : j["dynamicStatuses"]) {
                DynamicRuleStatus rs;
                read_opt(dj, "filename", rs.filename);
                read_opt(dj, "status", rs.status);
                read_opt(dj, "preview", rs.preview);
                read_opt(dj, "ok", rs.ok);
                out.dynamic_statuses.push_back(std::move(rs));
            }
        }
        read_opt(j, "discordStatus", out.discord_status);
        if (j.contains("twitch") && j["twitch"].is_object()) {
            const auto& tw = j["twitch"];
            read_opt(tw, "connected", out.twitch.connected);
            read_opt(tw, "isLive", out.twitch.is_live);
            read_opt(tw, "viewers", out.twitch.viewers);
            read_opt(tw, "broadcasterId", out.twitch.broadcaster_id);
            read_opt(tw, "currentTitle", out.twitch.current_title);
            read_opt(tw, "currentGame", out.twitch.current_game);
            read_opt(tw, "statusText", out.twitch.status_text);
        }
        read_opt(j, "daemonRamKb", out.daemon_ram_kb);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace osmv::cfg

