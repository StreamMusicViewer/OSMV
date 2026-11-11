#include "formatter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace osmv::fmt {

std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return str;
}

std::string to_upper_utf8(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x80) {
            if (c >= 'a' && c <= 'z') {
                out.push_back(static_cast<char>(c - ('a' - 'A')));
            } else {
                out.push_back(static_cast<char>(c));
            }
        } else if (c == 0xC3 && i + 1 < input.size()) {
            // Latin-1 Supplement lowercase (U+00E0..U+00FE, excluding 0xF7) -> uppercase (U+00C0..U+00DE)
            unsigned char c2 = static_cast<unsigned char>(input[i + 1]);
            out.push_back(static_cast<char>(c));
            if (c2 >= 0xA0 && c2 <= 0xBE && c2 != 0xB7) {
                out.push_back(static_cast<char>(c2 - 0x20));
            } else {
                out.push_back(static_cast<char>(c2));
            }
            ++i;
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

std::tm local_tm_now() {
    std::time_t now = std::time(nullptr);
    std::tm tm_val{};
#ifdef _WIN32
    localtime_s(&tm_val, &now);
#else
    localtime_r(&now, &tm_val);
#endif
    return tm_val;
}

namespace {
std::string two_digits(int64_t val) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lld", static_cast<long long>(val));
    return buf;
}

std::string format_num(int64_t val, bool pad) {
    if (pad) return two_digits(val);
    return std::to_string(val);
}

std::string format_duration_tokens(int64_t total_sec,
                                   const std::string& format_str,
                                   bool pad_zeros,
                                   bool allow_days,
                                   bool allow_total_minutes) {
    if (total_sec < 0) total_sec = 0;

    std::string out = format_str;
    const bool has_days_token = allow_days && (out.find("$d") != std::string::npos);

    int64_t days = 0;
    int64_t hours = 0;
    if (has_days_token) {
        days = total_sec / 86400;
        hours = (total_sec % 86400) / 3600;
    } else {
        hours = total_sec / 3600;
    }
    int64_t minutes = (total_sec % 3600) / 60;
    int64_t seconds = total_sec % 60;
    int64_t total_minutes = total_sec / 60;

    if (allow_total_minutes) {
        out = replace_all(std::move(out), "$totalminutes", std::to_string(total_minutes));
    }
    if (allow_days) {
        out = replace_all(std::move(out), "$d", std::to_string(days));
    }
    out = replace_all(std::move(out), "$h", format_num(hours, pad_zeros));
    out = replace_all(std::move(out), "$m", format_num(minutes, pad_zeros));
    out = replace_all(std::move(out), "$s", format_num(seconds, pad_zeros));
    return out;
}
} // namespace

std::pair<std::string, std::string> format_time_string(const std::string& format_str,
                                                       bool use_ampm,
                                                       const std::tm* override_tm) {
    std::tm tm_val = override_tm ? *override_tm : local_tm_now();

    int hour24 = tm_val.tm_hour;
    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;

    std::string ampm = (hour24 >= 12) ? "PM" : "AM";
    std::string h_str = two_digits(use_ampm ? hour12 : hour24);
    std::string m_str = two_digits(tm_val.tm_min);
    std::string s_str = two_digits(tm_val.tm_sec);

    std::string pc_preview = h_str + ":" + m_str + ":" + s_str;
    if (use_ampm) pc_preview += " " + ampm;

    std::string out = format_str;
    out = replace_all(std::move(out), "$h", h_str);
    out = replace_all(std::move(out), "$m", m_str);
    out = replace_all(std::move(out), "$s", s_str);
    out = replace_all(std::move(out), "$tt", use_ampm ? ampm : "");

    // Collapse trailing whitespace if $tt was empty and user left "$h:$m $tt"
    if (!use_ampm) {
        while (!out.empty() && out.back() == ' ') {
            out.pop_back();
        }
    }

    return {pc_preview, out};
}

std::string format_date_string(const std::string& date_format,
                               const std::string& output_format,
                               const std::string& lang,
                               const std::tm* override_tm) {
    static constexpr const char* kMonthsEn[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    static constexpr const char* kMonthsFr[12] = {
        "janvier", "f\xC3\xA9vrier", "mars", "avril", "mai", "juin",
        "juillet", "ao\xC3\xBBt", "septembre", "octobre", "novembre", "d\xC3\xA9""cembre"
    };
    static constexpr const char* kDaysEn[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    static constexpr const char* kDaysFr[7] = {
        "dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"
    };

    std::tm tm_val = override_tm ? *override_tm : local_tm_now();
    int year = tm_val.tm_year + 1900;
    int month_idx = std::clamp(tm_val.tm_mon, 0, 11);
    int day = tm_val.tm_mday;
    int wday_idx = std::clamp(tm_val.tm_wday, 0, 6);

    const bool is_fr = (lang == "fr");
    const char* month_name = is_fr ? kMonthsFr[month_idx] : kMonthsEn[month_idx];
    const char* day_name = is_fr ? kDaysFr[wday_idx] : kDaysEn[wday_idx];

    std::string dd = two_digits(day);
    std::string mm = two_digits(month_idx + 1);
    std::string yy = two_digits(year % 100);
    std::string yyyy = std::to_string(year);

    std::string date_value;
    if (date_format == "$dd-MM-yy") {
        date_value = dd + "-" + mm + "-" + yy;
    } else if (date_format == "$dd-MM-yyyy") {
        date_value = dd + "-" + mm + "-" + yyyy;
    } else if (date_format == "$dd MMMM yyyy") {
        date_value = dd + " " + month_name + " " + yyyy;
    } else if (date_format == "$dddd dd MMMM yyyy") {
        date_value = std::string(day_name) + " " + dd + " " + month_name + " " + yyyy;
    } else {
        // Fallback custom pattern replacements
        date_value = date_format;
        date_value = replace_all(std::move(date_value), "$dddd", day_name);
        date_value = replace_all(std::move(date_value), "$dd", dd);
        date_value = replace_all(std::move(date_value), "$MMMM", month_name);
        date_value = replace_all(std::move(date_value), "$MM", mm);
        date_value = replace_all(std::move(date_value), "$yyyy", yyyy);
        date_value = replace_all(std::move(date_value), "$yy", yy);
    }

    return replace_all(output_format, "$date", date_value);
}

CountdownResult format_countdown_string(int year, int month, int day,
                                        int hour, int minute, int second,
                                        const std::string& format_str,
                                        const std::string& done_text,
                                        bool pad_zeros,
                                        std::time_t now_override) {
    std::tm target{};
    target.tm_year = year - 1900;
    target.tm_mon = std::clamp(month, 1, 12) - 1;
    target.tm_mday = std::clamp(day, 1, 31);
    target.tm_hour = std::clamp(hour, 0, 23);
    target.tm_min = std::clamp(minute, 0, 59);
    target.tm_sec = std::clamp(second, 0, 59);
    target.tm_isdst = -1;

    std::time_t target_t = std::mktime(&target);
    std::time_t now_t = (now_override > 0) ? now_override : std::time(nullptr);

    CountdownResult res;
    if (target_t <= now_t || target_t == static_cast<std::time_t>(-1)) {
        res.reached = true;
        res.remaining_seconds = 0;
        res.output = done_text.empty() ? "00:00:00" : done_text;
        return res;
    }

    res.reached = false;
    res.remaining_seconds = static_cast<int64_t>(target_t - now_t);
    res.output = format_duration_tokens(res.remaining_seconds, format_str, pad_zeros, true, false);
    return res;
}

std::string format_chrono_down(int64_t remaining_ms,
                               const std::string& format_str,
                               const std::string& done_text,
                               bool pad_zeros) {
    if (remaining_ms <= 0) {
        return done_text.empty() ? "00:00:00" : done_text;
    }
    int64_t total_sec = (remaining_ms + 999) / 1000;
    return format_duration_tokens(total_sec, format_str, pad_zeros, true, false);
}

std::string format_chrono_up(int64_t elapsed_ms,
                             const std::string& format_str,
                             bool pad_zeros) {
    if (elapsed_ms < 0) elapsed_ms = 0;
    int64_t total_sec = elapsed_ms / 1000;
    return format_duration_tokens(total_sec, format_str, pad_zeros, true, true);
}

std::string format_now_playing(const std::string& title,
                               const std::string& artist,
                               const std::string& album,
                               const std::string& format_str) {
    std::string out = format_str;
    out = replace_all(std::move(out), "$artist_upper", to_upper_utf8(artist));
    out = replace_all(std::move(out), "$song_upper", to_upper_utf8(title));
    out = replace_all(std::move(out), "$album_upper", to_upper_utf8(album));
    out = replace_all(std::move(out), "$artist", artist);
    out = replace_all(std::move(out), "$song", title);
    out = replace_all(std::move(out), "$album", album);
    return out;
}

std::string format_rate_kbps(double bytes_per_sec) {
    if (bytes_per_sec < 0.0) bytes_per_sec = 0.0;
    double kbps = (bytes_per_sec * 8.0) / 1000.0;
    char buf[64];
    if (kbps >= 1000.0) {
        std::snprintf(buf, sizeof(buf), "%.1f Mbps", kbps / 1000.0);
    } else {
        std::snprintf(buf, sizeof(buf), "%.1f Kbps", kbps);
    }
    return buf;
}

std::string format_system_info(const SystemMetrics& metrics, const std::string& format_str) {
    char cpu_buf[32], ram_use_buf[32], ram_avail_buf[32];
    std::snprintf(cpu_buf, sizeof(cpu_buf), "%.0f %%", std::round(metrics.cpu_percent));
    std::snprintf(ram_use_buf, sizeof(ram_use_buf), "%.0f %%", std::round(metrics.ram_in_use_percent));
    std::snprintf(ram_avail_buf, sizeof(ram_avail_buf), "%.0f %%", std::round(metrics.ram_avail_percent));

    std::string out = format_str;
    out = replace_all(std::move(out), "$CPU_USAGE", cpu_buf);
    out = replace_all(std::move(out), "$RAM_MB_AVAIL", std::to_string(metrics.ram_avail_mb) + " MB");
    out = replace_all(std::move(out), "$RAM_%_IN_USE", ram_use_buf);
    out = replace_all(std::move(out), "$RAM_%_AVAIL", ram_avail_buf);
    out = replace_all(std::move(out), "$AMOUNT_PROCESSES", std::to_string(metrics.process_count));
    out = replace_all(std::move(out), "$UPLOAD", format_rate_kbps(metrics.upload_bytes_per_sec));
    out = replace_all(std::move(out), "$DOWNLOAD", format_rate_kbps(metrics.download_bytes_per_sec));
    return out;
}

std::string format_twitch_viewers(int viewers, bool is_live,
                                  const std::string& format_str,
                                  const std::string& offline_text) {
    if (!is_live) {
        return offline_text.empty() ? "Offline" : offline_text;
    }
    return replace_all(format_str, "$viewers", std::to_string(std::max(0, viewers)));
}

std::string base64_encode(const uint8_t* data, size_t len) {
    static constexpr char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t b0 = data[i];
        uint32_t b1 = (i + 1 < len) ? data[i + 1] : 0;
        uint32_t b2 = (i + 2 < len) ? data[i + 2] : 0;
        uint32_t triple = (b0 << 16) | (b1 << 8) | b2;
        out.push_back(kTable[(triple >> 18) & 0x3F]);
        out.push_back(kTable[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < len) ? kTable[(triple >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < len) ? kTable[triple & 0x3F] : '=');
    }
    return out;
}

std::string base64_encode(const std::vector<uint8_t>& data) {
    return base64_encode(data.data(), data.size());
}

std::vector<uint8_t> base64_decode(const std::string& b64) {
    static const auto kDec = []() {
        std::array<int8_t, 256> t{};
        t.fill(-1);
        const char* kTable =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; ++i) {
            t[static_cast<unsigned char>(kTable[i])] = static_cast<int8_t>(i);
        }
        return t;
    }();

    std::vector<uint8_t> out;
    out.reserve((b64.size() / 4) * 3);
    uint32_t val = 0;
    int bits = -8;
    for (unsigned char c : b64) {
        if (c == '=') break;
        int8_t d = kDec[c];
        if (d == -1) continue;
        val = (val << 6) + static_cast<uint32_t>(d);
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

std::string url_encode(const std::string& s) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 2);
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else if (c == ' ') {
            out.push_back('+');
        } else {
            out.push_back('%');
            out.push_back(kHex[(c >> 4) & 0xF]);
            out.push_back(kHex[c & 0xF]);
        }
    }
    return out;
}

std::string url_decode(const std::string& s) {
    auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int h1 = hex_val(s[i + 1]);
            int h2 = hex_val(s[i + 2]);
            if (h1 >= 0 && h2 >= 0) {
                out.push_back(static_cast<char>((h1 << 4) | h2));
                i += 2;
                continue;
            }
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string format_iso8601_utc(std::time_t t) {
    if (t == 0) t = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                  utc.tm_hour, utc.tm_min, utc.tm_sec);
    return buf;
}

} // namespace osmv::fmt

