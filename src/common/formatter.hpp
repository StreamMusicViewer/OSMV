#pragma once

#include "types.hpp"
#include <ctime>
#include <string>
#include <utility>
#include <vector>

namespace osmv::fmt {

std::string replace_all(std::string str, const std::string& from, const std::string& to);
std::string to_upper_utf8(const std::string& input);

// Time: returns { current_pc_time, formatted_output }
std::pair<std::string, std::string> format_time_string(const std::string& format_str,
                                                       bool use_ampm,
                                                       const std::tm* override_tm = nullptr);

// Date: supports $dd-MM-yy, $dd-MM-yyyy, $dd MMMM yyyy, $dddd dd MMMM yyyy and $date
std::string format_date_string(const std::string& date_format,
                               const std::string& output_format,
                               const std::string& lang,
                               const std::tm* override_tm = nullptr);

struct CountdownResult {
    bool reached = false;
    int64_t remaining_seconds = 0;
    std::string output;
};

CountdownResult format_countdown_string(int year, int month, int day,
                                        int hour, int minute, int second,
                                        const std::string& format_str,
                                        const std::string& done_text,
                                        bool pad_zeros,
                                        std::time_t now_override = 0);

std::string format_chrono_down(int64_t remaining_ms,
                               const std::string& format_str,
                               const std::string& done_text,
                               bool pad_zeros);

std::string format_chrono_up(int64_t elapsed_ms,
                             const std::string& format_str,
                             bool pad_zeros);

// Now Playing ($artist, $song, $album, $artist_upper, $song_upper, $album_upper)
std::string format_now_playing(const std::string& title,
                               const std::string& artist,
                               const std::string& album,
                               const std::string& format_str);

// System Info ($CPU_USAGE, $RAM_MB_AVAIL, $RAM_%_IN_USE, $RAM_%_AVAIL, $AMOUNT_PROCESSES, $UPLOAD, $DOWNLOAD)
std::string format_system_info(const SystemMetrics& metrics, const std::string& format_str);
std::string format_rate_kbps(double bytes_per_sec);

// Twitch ($viewers)
std::string format_twitch_viewers(int viewers, bool is_live,
                                  const std::string& format_str,
                                  const std::string& offline_text);

// Encoding / Time utilities
std::string base64_encode(const uint8_t* data, size_t len);
std::string base64_encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> base64_decode(const std::string& b64);
std::string url_encode(const std::string& s);
std::string url_decode(const std::string& s);
std::string extract_twitch_token(const std::string& raw);
std::string format_iso8601_utc(std::time_t t = 0);
std::tm local_tm_now();

} // namespace osmv::fmt

