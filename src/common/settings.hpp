#pragma once

#include "types.hpp"
#include <filesystem>
#include <string>
#include <unordered_map>

namespace osmv::cfg {

std::filesystem::path shared_dir();
std::filesystem::path settings_path();
std::filesystem::path current_song_path();
std::filesystem::path dynamic_text_dir(const AppSettings& s);
std::filesystem::path obs_widget_html_path();
std::filesystem::path games_custom_json_path();

AppSettings load_settings();
bool save_settings(const AppSettings& s);

// Atomic file write (writes .tmp then replaces target, falling back cleanly if locked)
bool write_file_bytes(const std::filesystem::path& path, const void* data, size_t size);
bool write_file_text(const std::filesystem::path& path, const std::string& content);

// Caches last written text per path so unchanged outputs don't rewrite disk every second
class CachedTextWriter {
public:
    void write_if_changed(const std::filesystem::path& path, const std::string& content);
    void remove_if_exists(const std::filesystem::path& path);
    void clear_cache();

private:
    std::unordered_map<std::string, std::string> cache_;
};

void write_current_song_json(const std::string& title,
                             const std::string& artist,
                             const std::string& album,
                             const std::string& status,
                             const std::string& thumbnail_b64,
                             bool dynamic_color,
                             bool audio_visualizer);
void write_null_song_json();
void write_snaz_track_info_json(const std::filesystem::path& dir,
                                 const std::string& player_name,
                                 const std::string& artist,
                                 const std::string& song,
                                 const std::string& album,
                                 const std::string& status);

// JSON conversions for AppSettings and DaemonStateSnapshot
std::string settings_to_json_string(const AppSettings& s, bool pretty = true);
bool settings_from_json_string(const std::string& json_str, AppSettings& out);

std::string snapshot_to_json_string(const DaemonStateSnapshot& snap);
bool snapshot_from_json_string(const std::string& json_str, DaemonStateSnapshot& out);

} // namespace osmv::cfg

