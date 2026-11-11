#include "daemon_main.hpp"
#include "discord_rpc.hpp"
#include "dynamic_files.hpp"
#include "media_provider.hpp"
#include "sysinfo.hpp"
#include "tray.hpp"
#include "twitch_client.hpp"

#include "common/formatter.hpp"
#include "common/http.hpp"
#include "common/ipc.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "common/types.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>

#if defined(__GLIBC__)
#include <malloc.h>
#endif

namespace osmv::daemon {

namespace {
using json = nlohmann::json;

void spawn_gui_process() {
#ifdef _WIN32
    os::spawn_detached(os::exe_path(), {"--gui"});
#else
    auto gui_bin = os::exe_dir() / "osmv-gui";
    std::error_code ec;
    if (std::filesystem::exists(gui_bin, ec)) {
        os::spawn_detached(gui_bin, {});
    } else {
        os::spawn_detached(os::exe_path(), {"--gui"});
    }
#endif
}

std::string load_thumbnail_base64_from_url(const std::string& url) {
    if (url.empty()) return {};
    if (url.rfind("file://", 0) == 0) {
        std::string raw_path = fmt::url_decode(url.substr(7));
        std::ifstream f(os::path_from_utf8(raw_path), std::ios::binary);
        if (!f) return {};
        std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (bytes.empty() || bytes.size() > 8 * 1024 * 1024) return {};
        return fmt::base64_encode(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
    }
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0) {
        auto resp = http::get(url, {}, 5000);
        if (resp.ok && !resp.body.empty()) {
            return fmt::base64_encode(reinterpret_cast<const uint8_t*>(resp.body.data()), resp.body.size());
        }
    }
    return {};
}

uint64_t fnv1a_hash(const std::string& s) {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

std::string update_gui_thumb_file(const std::string& song_key,
                                  const std::string& thumb_b64,
                                  std::string& prev_thumb_path) {
    if (song_key.empty() || thumb_b64.empty()) {
        if (!prev_thumb_path.empty()) {
            std::error_code ec;
            std::filesystem::remove(os::path_from_utf8(prev_thumb_path), ec);
            prev_thumb_path.clear();
        }
        return {};
    }

    char hex[32];
    std::snprintf(hex, sizeof(hex), "osmv_thumb_%016llx.img",
                  static_cast<unsigned long long>(fnv1a_hash(song_key)));
    auto target = os::temp_dir() / hex;
    std::string target_utf8 = os::path_to_utf8(target);

    if (target_utf8 != prev_thumb_path) {
        if (!prev_thumb_path.empty()) {
            std::error_code ec;
            std::filesystem::remove(os::path_from_utf8(prev_thumb_path), ec);
        }
        std::error_code ec;
        if (!std::filesystem::exists(target, ec)) {
            auto bytes = fmt::base64_decode(thumb_b64);
            if (!bytes.empty()) {
                cfg::write_file_bytes(target, bytes.data(), bytes.size());
            } else {
                return {};
            }
        }
        prev_thumb_path = target_utf8;
    }
    return target_utf8;
}

struct TimerRuntime {
    bool chrono_down_running = false;
    int64_t chrono_down_remaining_ms = 0;
    int64_t chrono_down_configured_ms = 0;

    bool chrono_up_running = false;
    int64_t chrono_up_elapsed_ms = 0;
    int64_t chrono_up_configured_ms = 0;

    std::chrono::steady_clock::time_point last_tick{};
};

class DaemonApp {
public:
    int run(bool silent_start) {
        if (!instance_guard_.try_acquire("osmv-daemon-instance-v3")) {
            // Already running: notify existing daemon to open GUI
            std::string dummy;
            ipc::send_request(R"({"cmd":"show_gui"})", dummy, 500);
            return 0;
        }

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            settings_ = cfg::load_settings();
            reset_chrono_down_locked();
            reset_chrono_up_locked();
            timers_.last_tick = std::chrono::steady_clock::now();
        }

        tray_ = create_tray_controller();
        tray_->set_language(settings_.general.language);
        tray_->init(TrayCallbacks{
            .on_show_gui = []() { spawn_gui_process(); },
            .on_quit = [this]() { request_shutdown(); }
        });

        discord_.start();
        ipc_.start([this](const std::string& req) { return handle_ipc(req); });

        running_.store(true);
        local_thread_ = std::thread(&DaemonApp::local_loop, this);
        net_thread_ = std::thread(&DaemonApp::network_loop, this);

        if (!silent_start && settings_.general.open_gui_on_startup) {
            spawn_gui_process();
        }

        tray_->run_event_loop();

        // Cleanup after tray exits
        request_shutdown();
        if (local_thread_.joinable()) local_thread_.join();
        if (net_thread_.joinable()) net_thread_.join();
        ipc_.stop();
        discord_.stop();

        if (!prev_thumb_path_.empty()) {
            std::error_code ec;
            std::filesystem::remove(os::path_from_utf8(prev_thumb_path_), ec);
        }
        cfg::write_null_song_json();
        return 0;
    }

private:
    void wake_workers() {
        {
            std::lock_guard<std::mutex> lock(cv_mutex_);
            force_refresh_.store(true);
            force_net_refresh_.store(true);
        }
        cv_.notify_all();
    }

    void request_shutdown() {
        if (!running_.exchange(false)) return;
        wake_workers();
        if (tray_) tray_->request_quit();
    }

    int64_t chrono_down_initial_ms(const ChronoDownSettings& cd) const {
        return (static_cast<int64_t>(std::max(0, cd.initial_hours)) * 3600LL +
                static_cast<int64_t>(std::max(0, cd.initial_minutes)) * 60LL +
                static_cast<int64_t>(std::max(0, cd.initial_seconds))) * 1000LL;
    }

    int64_t chrono_up_start_ms(const ChronoUpSettings& cu) const {
        return (static_cast<int64_t>(std::max(0, cu.start_hours)) * 3600LL +
                static_cast<int64_t>(std::max(0, cu.start_minutes)) * 60LL +
                static_cast<int64_t>(std::max(0, cu.start_seconds))) * 1000LL;
    }

    void reset_chrono_down_locked() {
        timers_.chrono_down_running = false;
        timers_.chrono_down_configured_ms = chrono_down_initial_ms(settings_.chrono_down);
        timers_.chrono_down_remaining_ms = timers_.chrono_down_configured_ms;
    }

    void reset_chrono_up_locked() {
        timers_.chrono_up_running = false;
        timers_.chrono_up_configured_ms = chrono_up_start_ms(settings_.chrono_up);
        timers_.chrono_up_elapsed_ms = timers_.chrono_up_configured_ms;
    }

    std::string handle_ipc(const std::string& req_str) {
        try {
            json req = json::parse(req_str);
            std::string cmd = req.value("cmd", "");

            if (cmd == "show_gui") {
                spawn_gui_process();
                return R"({"ok":true})";
            }
            if (cmd == "quit") {
                request_shutdown();
                return R"({"ok":true})";
            }
            if (cmd == "reload_settings") {
                AppSettings new_s = cfg::load_settings();
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    int64_t new_cd_ms = chrono_down_initial_ms(new_s.chrono_down);
                    if (!timers_.chrono_down_running && new_cd_ms != timers_.chrono_down_configured_ms) {
                        timers_.chrono_down_configured_ms = new_cd_ms;
                        timers_.chrono_down_remaining_ms = new_cd_ms;
                    }
                    int64_t new_cu_ms = chrono_up_start_ms(new_s.chrono_up);
                    if (!timers_.chrono_up_running && new_cu_ms != timers_.chrono_up_configured_ms) {
                        timers_.chrono_up_configured_ms = new_cu_ms;
                        timers_.chrono_up_elapsed_ms = new_cu_ms;
                    }
                    settings_ = std::move(new_s);
                    if (tray_) tray_->set_language(settings_.general.language);
                }
                wake_workers();
                return R"({"ok":true})";
            }
            if (cmd == "get_state") {
                DaemonStateSnapshot snap;
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    last_gui_poll_ = std::chrono::steady_clock::now();
                    snap = snapshot_;
                }
                snap.discord_status = discord_.status_string();
                snap.daemon_ram_kb = os::get_process_memory_kb();
                return json({{"ok", true}, {"state", json::parse(cfg::snapshot_to_json_string(snap))}}).dump();
            }
            if (cmd == "chrono_down") {
                std::string action = req.value("action", "");
                std::lock_guard<std::mutex> lock(state_mutex_);
                if (action == "start") {
                    if (timers_.chrono_down_remaining_ms <= 0) {
                        timers_.chrono_down_remaining_ms = chrono_down_initial_ms(settings_.chrono_down);
                    }
                    timers_.chrono_down_running = true;
                } else if (action == "pause") {
                    timers_.chrono_down_running = false;
                } else if (action == "reset") {
                    reset_chrono_down_locked();
                }
                wake_workers();
                return R"({"ok":true})";
            }
            if (cmd == "chrono_up") {
                std::string action = req.value("action", "");
                std::lock_guard<std::mutex> lock(state_mutex_);
                if (action == "start") {
                    timers_.chrono_up_running = true;
                } else if (action == "pause") {
                    timers_.chrono_up_running = false;
                } else if (action == "reset") {
                    reset_chrono_up_locked();
                }
                wake_workers();
                return R"({"ok":true})";
            }
            if (cmd == "textline_next") {
                force_textline_next_.store(true);
                wake_workers();
                return R"({"ok":true})";
            }
            if (cmd == "twitch_search_games") {
                TwitchSettings tw;
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    tw = settings_.twitch;
                }
                std::string query = req.value("query", "");
                auto games = twitch_.search_categories(tw, query);
                json arr = json::array();
                for (const auto& g : games) {
                    arr.push_back({{"id", g.id}, {"name", g.name}});
                }
                return json({{"ok", true}, {"games", std::move(arr)}}).dump();
            }
            if (cmd == "twitch_update_channel") {
                TwitchSettings tw;
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    tw = settings_.twitch;
                }
                std::string title = req.value("title", "");
                std::string game_id = req.value("game_id", "");
                std::string err;
                bool ok = twitch_.update_channel_info(tw, title, game_id, err);
                return json({{"ok", ok}, {"error", err}}).dump();
            }
        } catch (...) {
        }
        return R"({"ok":false,"error":"Invalid request"})";
    }

    void local_loop() {
        auto media = create_media_provider();
        auto sampler = create_system_sampler();
        cfg::CachedTextWriter text_writer;
        std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));

        std::filesystem::file_time_type last_settings_mtime{};
        std::string last_song_key;
        std::string cached_thumb_b64;
        std::string last_json_sig;
        std::string last_snaz_sig;
        bool json_exists_or_written = false;

        int textline_idx = 0;
        std::chrono::steady_clock::time_point next_textline{};
        std::chrono::steady_clock::time_point next_sysinfo{};
        std::chrono::steady_clock::time_point next_media_poll{};
        PolledMedia last_polled_media;
        last_polled_media.song.status = "closed";
        SystemMetrics last_metrics;

        while (running_.load()) {
            auto now_steady = std::chrono::steady_clock::now();

            // 1. Hot-reload settings if file changed externally
            {
                std::error_code ec;
                auto sp = cfg::settings_path();
                auto mtime = std::filesystem::last_write_time(sp, ec);
                if (!ec && mtime != last_settings_mtime) {
                    last_settings_mtime = mtime;
                    AppSettings loaded = cfg::load_settings();
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    settings_ = std::move(loaded);
                    if (tray_) tray_->set_language(settings_.general.language);
                }
            }

            AppSettings cfg_copy;
            bool gui_active = false;
            int64_t cd_rem = 0, cu_el = 0;
            bool cd_run = false, cu_run = false;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                // Advance chronos smoothly
                int64_t dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now_steady - timers_.last_tick).count();
                if (dt_ms < 0) dt_ms = 0;
                timers_.last_tick = now_steady;

                if (timers_.chrono_down_running) {
                    timers_.chrono_down_remaining_ms = std::max<int64_t>(0, timers_.chrono_down_remaining_ms - dt_ms);
                    if (timers_.chrono_down_remaining_ms == 0) {
                        timers_.chrono_down_running = false;
                    }
                }
                if (timers_.chrono_up_running) {
                    timers_.chrono_up_elapsed_ms += dt_ms;
                }

                cfg_copy = settings_;
                cd_rem = timers_.chrono_down_remaining_ms;
                cd_run = timers_.chrono_down_running;
                cu_el = timers_.chrono_up_elapsed_ms;
                cu_run = timers_.chrono_up_running;
                gui_active = (now_steady - last_gui_poll_) < std::chrono::seconds(4);
            }

            const auto dyn_dir = cfg::dynamic_text_dir(cfg_copy);

            // 2. Media polling (every ~1s when enabled, or Discord music RPC enabled, or GUI open)
            const bool need_media = cfg_copy.now_playing_enabled ||
                                    (cfg_copy.discord.enabled && cfg_copy.discord.use_music_when_playing) ||
                                    gui_active;
            if (need_media && now_steady >= next_media_poll) {
                next_media_poll = now_steady + std::chrono::seconds(1);
                last_polled_media = media->current_song();
            } else if (!need_media) {
                last_polled_media = PolledMedia{};
                last_polled_media.song.status = "closed";
            }

            const SongInfo& song = last_polled_media.song;
            const bool is_inactive = (song.status == "closed" || song.status == "stopped" || song.title.empty());

            // 3. Discord RPC update
            if (cfg_copy.discord.enabled) {
                if (!is_inactive) {
                    discord_.post_update(cfg_copy.discord, song.title, song.artist, song.album, song.art_url, song.status);
                } else {
                    discord_.post_update(cfg_copy.discord, std::nullopt, std::nullopt, std::nullopt, std::nullopt, "idle");
                }
            } else {
                discord_.post_update(cfg_copy.discord, std::nullopt, std::nullopt, std::nullopt, std::nullopt, "disabled");
            }

            // 4. Now Playing JSON + Snaz text files + GUI thumbnail
            std::string thumb_file;
            std::string np_preview;
            if (!cfg_copy.now_playing_enabled) {
                if (json_exists_or_written) {
                    std::error_code ec;
                    std::filesystem::remove(cfg::current_song_path(), ec);
                    json_exists_or_written = false;
                }
                last_song_key.clear();
                cached_thumb_b64.clear();
                last_json_sig.clear();
                thumb_file = update_gui_thumb_file("", "", prev_thumb_path_);
                text_writer.remove_if_exists(dyn_dir / "TrackArtist.txt");
                text_writer.remove_if_exists(dyn_dir / "TrackSong.txt");
                text_writer.remove_if_exists(dyn_dir / "TrackAlbum.txt");
                text_writer.remove_if_exists(dyn_dir / "PlayingNow.txt");
                text_writer.remove_if_exists(dyn_dir / "track-info.json");
            } else if (is_inactive) {
                if (last_json_sig != "null") {
                    cfg::write_null_song_json();
                    last_json_sig = "null";
                    json_exists_or_written = true;
                }
                thumb_file = update_gui_thumb_file("", "", prev_thumb_path_);

                if (cfg_copy.now_playing.export_text_files) {
                    std::string fb = cfg_copy.now_playing.clear_when_inactive ? "" : cfg_copy.now_playing.fallback_text;
                    text_writer.write_if_changed(dyn_dir / "TrackArtist.txt", fb);
                    text_writer.write_if_changed(dyn_dir / "TrackSong.txt", fb);
                    text_writer.write_if_changed(dyn_dir / "TrackAlbum.txt", fb);
                } else {
                    text_writer.remove_if_exists(dyn_dir / "TrackArtist.txt");
                    text_writer.remove_if_exists(dyn_dir / "TrackSong.txt");
                    text_writer.remove_if_exists(dyn_dir / "TrackAlbum.txt");
                }
                if (cfg_copy.now_playing.export_formatted_file) {
                    np_preview = cfg_copy.now_playing.clear_when_inactive ? "" : cfg_copy.now_playing.fallback_text;
                    text_writer.write_if_changed(dyn_dir / "PlayingNow.txt", np_preview);
                } else {
                    text_writer.remove_if_exists(dyn_dir / "PlayingNow.txt");
                }
            } else {
                std::string key = song.title + "|" + song.artist + "|" + song.art_url;
                if (key != last_song_key) {
                    last_song_key = key;
                    if (!song.thumbnail_b64.empty()) {
                        cached_thumb_b64 = song.thumbnail_b64;
                    } else if (!song.art_url.empty()) {
                        cached_thumb_b64 = load_thumbnail_base64_from_url(song.art_url);
                    } else {
                        cached_thumb_b64.clear();
                    }
                }
                if (gui_active) {
                    thumb_file = update_gui_thumb_file(key, cached_thumb_b64, prev_thumb_path_);
                }

                std::string json_sig = key + "|" + song.album + "|" + song.status + "|" +
                                       (cfg_copy.dynamic_color ? "1" : "0") + "|" +
                                       (cfg_copy.audio_visualizer ? "1" : "0") + "|" +
                                       std::to_string(cached_thumb_b64.size());
                if (json_sig != last_json_sig) {
                    cfg::write_current_song_json(
                        song.title, song.artist, song.album, song.status, cached_thumb_b64,
                        cfg_copy.dynamic_color, cfg_copy.audio_visualizer);
                    last_json_sig = std::move(json_sig);
                    json_exists_or_written = true;
                }

                if (cfg_copy.now_playing.export_text_files) {
                    text_writer.write_if_changed(dyn_dir / "TrackArtist.txt", song.artist);
                    text_writer.write_if_changed(dyn_dir / "TrackSong.txt", song.title);
                    text_writer.write_if_changed(dyn_dir / "TrackAlbum.txt", song.album);
                } else {
                    text_writer.remove_if_exists(dyn_dir / "TrackArtist.txt");
                    text_writer.remove_if_exists(dyn_dir / "TrackSong.txt");
                    text_writer.remove_if_exists(dyn_dir / "TrackAlbum.txt");
                }

                np_preview = fmt::format_now_playing(song.title, song.artist, song.album, cfg_copy.now_playing.custom_format);
                if (cfg_copy.now_playing.export_formatted_file) {
                    text_writer.write_if_changed(dyn_dir / "PlayingNow.txt", np_preview);
                } else {
                    text_writer.remove_if_exists(dyn_dir / "PlayingNow.txt");
                }

                if (cfg_copy.now_playing.export_snaz_json) {
                    std::string snaz_sig = last_polled_media.player_name + "|" + song.artist + "|" + song.title + "|" + song.album + "|" + song.status;
                    if (snaz_sig != last_snaz_sig) {
                        cfg::write_snaz_track_info_json(
                            dyn_dir, last_polled_media.player_name, song.artist, song.title, song.album, song.status);
                        last_snaz_sig = std::move(snaz_sig);
                    }
                } else {
                    last_snaz_sig.clear();
                    text_writer.remove_if_exists(dyn_dir / "track-info.json");
                }
            }

            // 5. Time & Date
            auto [pc_time, time_out] = fmt::format_time_string(cfg_copy.time.format, cfg_copy.time.use_ampm);
            if (cfg_copy.time.enabled) {
                text_writer.write_if_changed(dyn_dir / "Time.txt", time_out);
            } else {
                text_writer.remove_if_exists(dyn_dir / "Time.txt");
            }

            std::string date_out = fmt::format_date_string(
                cfg_copy.date.date_format, cfg_copy.date.output_format, cfg_copy.general.language);
            if (cfg_copy.date.enabled) {
                text_writer.write_if_changed(dyn_dir / "Date.txt", date_out);
            } else {
                text_writer.remove_if_exists(dyn_dir / "Date.txt");
            }

            // 6. Countdown, Chrono Down, Chrono Up
            auto cd_res = fmt::format_countdown_string(
                cfg_copy.countdown.target_year, cfg_copy.countdown.target_month, cfg_copy.countdown.target_day,
                cfg_copy.countdown.target_hour, cfg_copy.countdown.target_minute, cfg_copy.countdown.target_second,
                cfg_copy.countdown.format, cfg_copy.countdown.done_text, cfg_copy.countdown.pad_zeros);
            if (cfg_copy.countdown.enabled) {
                text_writer.write_if_changed(dyn_dir / "Countdown.txt", cd_res.output);
            } else {
                text_writer.remove_if_exists(dyn_dir / "Countdown.txt");
            }

            std::string cdown_out = fmt::format_chrono_down(
                cd_rem, cfg_copy.chrono_down.format, cfg_copy.chrono_down.done_text, cfg_copy.chrono_down.pad_zeros);
            if (cfg_copy.chrono_down.enabled) {
                text_writer.write_if_changed(dyn_dir / "ChronoDown.txt", cdown_out);
            } else {
                text_writer.remove_if_exists(dyn_dir / "ChronoDown.txt");
            }

            std::string cup_out = fmt::format_chrono_up(
                cu_el, cfg_copy.chrono_up.format, cfg_copy.chrono_up.pad_zeros);
            if (cfg_copy.chrono_up.enabled) {
                text_writer.write_if_changed(dyn_dir / "ChronoUp.txt", cup_out);
            } else {
                text_writer.remove_if_exists(dyn_dir / "ChronoUp.txt");
            }

            // 7. System Info
            if (cfg_copy.system_info.enabled || gui_active) {
                if (now_steady >= next_sysinfo) {
                    int iv = cfg_copy.system_info.enabled ? std::clamp(cfg_copy.system_info.interval_sec, 1, 60) : 1;
                    next_sysinfo = now_steady + std::chrono::seconds(iv);
                    last_metrics = sampler->sample();
                }
            }
            std::string sys_out = fmt::format_system_info(last_metrics, cfg_copy.system_info.format);
            if (cfg_copy.system_info.enabled) {
                text_writer.write_if_changed(dyn_dir / "SystemInfo.txt", sys_out);
            } else {
                text_writer.remove_if_exists(dyn_dir / "SystemInfo.txt");
            }

            // 8. Textline Changer
            std::string tl_out;
            if (cfg_copy.textline.enabled && !cfg_copy.textline.lines.empty()) {
                std::vector<std::string> valid_lines;
                valid_lines.reserve(cfg_copy.textline.lines.size());
                for (const auto& l : cfg_copy.textline.lines) {
                    if (!cfg_copy.textline.skip_empty || !l.empty()) {
                        valid_lines.push_back(l);
                    }
                }
                if (!valid_lines.empty()) {
                    bool do_advance = force_textline_next_.exchange(false);
                    if (now_steady >= next_textline) do_advance = true;

                    if (do_advance) {
                        next_textline = now_steady + std::chrono::seconds(std::clamp(cfg_copy.textline.interval_sec, 1, 3600));
                        if (cfg_copy.textline.random_order && valid_lines.size() > 1) {
                            std::uniform_int_distribution<int> dist(0, static_cast<int>(valid_lines.size()) - 1);
                            int next_i = dist(rng);
                            if (next_i == textline_idx) next_i = (next_i + 1) % static_cast<int>(valid_lines.size());
                            textline_idx = next_i;
                        } else {
                            textline_idx = (textline_idx + 1) % static_cast<int>(valid_lines.size());
                        }
                    }
                    if (textline_idx >= static_cast<int>(valid_lines.size())) textline_idx = 0;
                    tl_out = valid_lines[static_cast<size_t>(textline_idx)];
                    std::string fname = cfg_copy.textline.filename.empty() ? "TextlineChanger.txt" : cfg_copy.textline.filename;
                    text_writer.write_if_changed(dyn_dir / fname, tl_out);
                }
            } else if (!cfg_copy.textline.filename.empty()) {
                text_writer.remove_if_exists(dyn_dir / cfg_copy.textline.filename);
            }

            // 9. Update snapshot for GUI IPC
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                snapshot_.title = song.title;
                snapshot_.artist = song.artist;
                snapshot_.album = song.album;
                snapshot_.status = song.status;
                snapshot_.thumbnail_file_path = thumb_file;
                snapshot_.playing_now_preview = np_preview;

                snapshot_.time_pc_preview = pc_time;
                snapshot_.time_live_preview = time_out;
                snapshot_.date_live_preview = date_out;

                snapshot_.countdown_preview = cd_res.output;
                snapshot_.countdown_reached = cd_res.reached;
                snapshot_.chrono_down_preview = cdown_out;
                snapshot_.chrono_down_running = cd_run;
                snapshot_.chrono_down_remaining_ms = cd_rem;
                snapshot_.chrono_up_preview = cup_out;
                snapshot_.chrono_up_running = cu_run;
                snapshot_.chrono_up_elapsed_ms = cu_el;

                snapshot_.sys_metrics = last_metrics;
                snapshot_.sysinfo_preview = sys_out;

                snapshot_.textline_preview = tl_out;
                snapshot_.textline_index = textline_idx;
            }

#if defined(__GLIBC__)
            malloc_trim(0);
#endif
            std::unique_lock<std::mutex> cv_lock(cv_mutex_);
            cv_.wait_for(cv_lock, std::chrono::milliseconds(250), [&] {
                return !running_.load() || force_refresh_.exchange(false);
            });
        }
    }

    void network_loop() {
        while (running_.load()) {
            AppSettings cfg_copy;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                cfg_copy = settings_;
            }

            const auto dyn_dir = cfg::dynamic_text_dir(cfg_copy);
            dyn_files_.tick(cfg_copy.dynamic_files, dyn_dir);
            twitch_.tick(cfg_copy.twitch, dyn_dir);

            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                snapshot_.dynamic_statuses = dyn_files_.statuses();
                snapshot_.twitch = twitch_.status();
            }

            std::unique_lock<std::mutex> cv_lock(cv_mutex_);
            cv_.wait_for(cv_lock, std::chrono::milliseconds(1000), [&] {
                return !running_.load() || force_net_refresh_.exchange(false);
            });
        }
    }

    os::SingleInstanceGuard instance_guard_;
    std::unique_ptr<TrayController> tray_;
    DiscordRpcManager discord_;
    ipc::IpcServer ipc_;
    DynamicFilesRunner dyn_files_;
    TwitchClient twitch_;

    std::mutex state_mutex_;
    AppSettings settings_;
    TimerRuntime timers_;
    DaemonStateSnapshot snapshot_;
    std::chrono::steady_clock::time_point last_gui_poll_{};

    std::string prev_thumb_path_;
    std::atomic<bool> force_textline_next_{false};
    std::atomic<bool> force_refresh_{false};
    std::atomic<bool> force_net_refresh_{false};
    std::atomic<bool> running_{false};
    std::mutex cv_mutex_;
    std::condition_variable cv_;
    std::thread local_thread_;
    std::thread net_thread_;
};

} // namespace

int run_daemon(bool silent_start) {
    DaemonApp app;
    return app.run(silent_start);
}

} // namespace osmv::daemon
