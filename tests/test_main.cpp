#include "common/formatter.hpp"
#include "common/settings.hpp"
#include "common/types.hpp"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace osmv;

#define CHECK(cond)                                                                         \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            std::cerr << "FAIL at line " << __LINE__ << ": " << #cond << "\n";              \
            return 1;                                                                       \
        }                                                                                   \
    } while (0)

int main() {
    // 1. Time formatter
    {
        std::tm tm_pm{};
        tm_pm.tm_hour = 16;
        tm_pm.tm_min = 5;
        tm_pm.tm_sec = 9;
        auto [pc24, out24] = fmt::format_time_string("$h:$m:$s", false, &tm_pm);
        CHECK(pc24 == "16:05:09");
        CHECK(out24 == "16:05:09");

        auto [pc12, out12] = fmt::format_time_string("$h:$m:$s $tt", true, &tm_pm);
        CHECK(pc12 == "04:05:09 PM");
        CHECK(out12 == "04:05:09 PM");

        std::tm tm_mid{};
        tm_mid.tm_hour = 0;
        tm_mid.tm_min = 0;
        tm_mid.tm_sec = 1;
        auto [pc0, out0] = fmt::format_time_string("$h:$m $tt", true, &tm_mid);
        CHECK(pc0 == "12:00:01 AM");
        CHECK(out0 == "12:00 AM");
    }

    // 2. Date formatter (FR & EN)
    {
        std::tm tm_d{};
        tm_d.tm_year = 2013 - 1900;
        tm_d.tm_mon = 5; // June
        tm_d.tm_mday = 25;
        tm_d.tm_wday = 2; // Tuesday

        CHECK(fmt::format_date_string("$dd-MM-yy", "$date", "en", &tm_d) == "25-06-13");
        CHECK(fmt::format_date_string("$dd-MM-yyyy", "$date", "en", &tm_d) == "25-06-2013");
        CHECK(fmt::format_date_string("$dd MMMM yyyy", "Today: $date", "en", &tm_d) == "Today: 25 June 2013");
        CHECK(fmt::format_date_string("$dddd dd MMMM yyyy", "$date", "en", &tm_d) == "Tuesday 25 June 2013");
        CHECK(fmt::format_date_string("$dddd dd MMMM yyyy", "$date", "fr", &tm_d) == "mardi 25 juin 2013");
    }

    // 3. Countdown, Chrono Down, Chrono Up
    {
        std::tm target{};
        target.tm_year = 2026 - 1900;
        target.tm_mon = 0;
        target.tm_mday = 2;
        target.tm_hour = 12;
        target.tm_min = 0;
        target.tm_sec = 0;
        target.tm_isdst = -1;
        std::time_t target_t = std::mktime(&target);

        auto cd = fmt::format_countdown_string(2026, 1, 2, 12, 0, 0, "$d d $h:$m:$s", "Done!", true, target_t - 90061);
        CHECK(!cd.reached);
        CHECK(cd.remaining_seconds == 90061);
        CHECK(cd.output == "1 d 01:01:01");

        auto cd_done = fmt::format_countdown_string(2026, 1, 2, 12, 0, 0, "$h:$m:$s", "Live!", true, target_t + 10);
        CHECK(cd_done.reached);
        CHECK(cd_done.output == "Live!");

        CHECK(fmt::format_chrono_down(0, "$h:$m:$s", "Break over", true) == "Break over");
        CHECK(fmt::format_chrono_down(65100, "$h:$m:$s", "Break over", true) == "00:01:06");

        // 1h 15m 35s = 4535s
        CHECK(fmt::format_chrono_up(4535000, "$totalminutes:$s", true) == "75:35");
        CHECK(fmt::format_chrono_up(4535000, "$h:$m:$s", false) == "1:15:35");
        CHECK(fmt::format_chrono_up((2 * 86400 + 3 * 3600 + 4 * 60 + 5) * 1000LL, "$d days $h:$m:$s", true) == "2 days 03:04:05");
    }

    // 4. Now Playing formatter + UTF-8 upper
    {
        std::string s = fmt::format_now_playing("Été", "Café", "Rêve", "$artist_upper - '$song' ($album_upper)");
        CHECK(s == "CAFÉ - 'Été' (RÊVE)");
    }

    // 5. System Info formatter
    {
        SystemMetrics m;
        m.cpu_percent = 12.4;
        m.ram_avail_mb = 8192;
        m.ram_in_use_percent = 48.2;
        m.ram_avail_percent = 51.8;
        m.process_count = 195;
        m.upload_bytes_per_sec = 1600.0;   // 12.8 Kbps
        m.download_bytes_per_sec = 250000.0; // 2.0 Mbps

        std::string out = fmt::format_system_info(
            m, "$CPU_USAGE | $RAM_MB_AVAIL | $RAM_%_IN_USE | $RAM_%_AVAIL | $AMOUNT_PROCESSES | $UPLOAD | $DOWNLOAD");
        CHECK(out == "12 % | 8192 MB | 48 % | 52 % | 195 | 12.8 Kbps | 2.0 Mbps");
    }

    // 6. Twitch viewers
    {
        CHECK(fmt::format_twitch_viewers(1423, true, "Viewers: $viewers", "Offline") == "Viewers: 1423");
        CHECK(fmt::format_twitch_viewers(1423, false, "Viewers: $viewers", "Hors ligne") == "Hors ligne");
    }

    // 7. Base64 & URL encoding
    {
        std::string orig = "Hello OSMV v3! \xC3\xA9";
        std::string b64 = fmt::base64_encode(reinterpret_cast<const uint8_t*>(orig.data()), orig.size());
        auto dec = fmt::base64_decode(b64);
        CHECK(std::string(dec.begin(), dec.end()) == orig);
        CHECK(fmt::url_decode("file%20name%2Btest.png") == "file name+test.png");
    }

    // 8. Backward-compatible settings load & roundtrip
    {
        const char* v2_json = R"({
            "nowPlayingEnabled": false,
            "dynamicColor": true,
            "audioVisualizer": true,
            "discord": {
                "enabled": true,
                "client_id": "999888777",
                "status_key_playing": "play_custom"
            },
            "time": {
                "enabled": true,
                "format": "$h:$m",
                "use_ampm": true
            }
        })";
        AppSettings s;
        CHECK(cfg::settings_from_json_string(v2_json, s));
        CHECK(!s.now_playing_enabled);
        CHECK(s.dynamic_color);
        CHECK(s.audio_visualizer);
        CHECK(s.discord.enabled);
        CHECK(s.discord.client_id == "999888777");
        CHECK(s.discord.status_key_playing == "play_custom");
        CHECK(s.time.enabled);
        CHECK(s.time.format == "$h:$m");
        CHECK(s.time.use_ampm);

        s.textline.filename = "../../bad/path.txt";
        s.dynamic_files.rules.push_back(DynamicFileRule{true, "../img.png", "https://example.com", "image", 30, ""});
        std::string serialized = cfg::settings_to_json_string(s, false);
        AppSettings s2;
        CHECK(cfg::settings_from_json_string(serialized, s2));
        CHECK(s2.textline.filename.find("..") != 0 || s2.textline.filename.find('/') == std::string::npos);
        CHECK(s2.dynamic_files.rules.size() == 1);
        CHECK(s2.dynamic_files.rules[0].filename.find('/') == std::string::npos);
    }

    // 9. CachedTextWriter
    {
        auto tmp = std::filesystem::temp_directory_path() / "osmv_test_writer.txt";
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        cfg::CachedTextWriter w;
        w.write_if_changed(tmp, "hello");
        CHECK(std::filesystem::exists(tmp));
        w.write_if_changed(tmp, "hello");
        w.remove_if_exists(tmp);
        CHECK(!std::filesystem::exists(tmp));
    }

    std::cout << "All OSMV C++ unit tests passed!\n";
    return 0;
}

