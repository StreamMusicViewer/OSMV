#include "twitch_client.hpp"
#include "common/formatter.hpp"
#include "common/http.hpp"
#include "common/settings.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>

namespace osmv::daemon {

namespace {
using json = nlohmann::json;
} // namespace

std::vector<std::pair<std::string, std::string>> TwitchClient::auth_headers(const TwitchSettings& settings) const {
    return {
        {"Client-Id", settings.client_id},
        {"Authorization", "Bearer " + fmt::extract_twitch_token(settings.oauth_token)},
        {"Content-Type", "application/json"}
    };
}

bool TwitchClient::ensure_broadcaster_id(const TwitchSettings& settings) {
    if (settings.client_id.empty() || settings.oauth_token.empty() || settings.channel.empty()) {
        status_.connected = false;
        status_.status_text = "Missing Client ID / Token / Channel";
        return false;
    }

    if (!status_.broadcaster_id.empty() &&
        cached_channel_ == settings.channel &&
        cached_client_id_ == settings.client_id &&
        cached_token_ == settings.oauth_token) {
        return true;
    }

    std::string url = "https://api.twitch.tv/helix/users?login=" + fmt::url_encode(settings.channel);
    auto resp = http::get(url, auth_headers(settings), 5000);
    if (!resp.ok) {
        status_.connected = false;
        status_.broadcaster_id.clear();
        status_.status_text = resp.status_code == 401 ? "Invalid/Expired OAuth Token (401)" : resp.error;
        return false;
    }

    try {
        json j = json::parse(resp.body);
        if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
            status_.broadcaster_id = j["data"][0].value("id", "");
            cached_channel_ = settings.channel;
            cached_client_id_ = settings.client_id;
            cached_token_ = settings.oauth_token;
            return !status_.broadcaster_id.empty();
        }
        status_.connected = false;
        status_.status_text = "Channel not found";
        return false;
    } catch (...) {
        status_.connected = false;
        status_.status_text = "Invalid JSON response";
        return false;
    }
}

void TwitchClient::tick(const TwitchSettings& settings, const std::filesystem::path& out_dir) {
    if (!settings.enabled) {
        status_ = TwitchStatus{};
        status_.status_text = "Disabled";
        text_writer_.clear_if_exists(out_dir / "TwitchViewers.txt");
        return;
    }

    if (cached_channel_ != settings.channel ||
        cached_client_id_ != settings.client_id ||
        cached_token_ != settings.oauth_token) {
        next_poll_ = std::chrono::steady_clock::time_point{};
    }

    auto now = std::chrono::steady_clock::now();
    if (now < next_poll_) return;
    next_poll_ = now + std::chrono::seconds(std::clamp(settings.interval_sec, 15, 3600));

    if (!ensure_broadcaster_id(settings)) return;

    auto hdrs = auth_headers(settings);
    // 1. Stream viewers
    {
        std::string url = "https://api.twitch.tv/helix/streams?user_id=" + status_.broadcaster_id;
        auto resp = http::get(url, hdrs, 5000);
        if (resp.ok) {
            try {
                json j = json::parse(resp.body);
                if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
                    status_.is_live = true;
                    status_.viewers = j["data"][0].value("viewer_count", 0);
                    status_.status_text = "Live (" + std::to_string(status_.viewers) + " viewers)";
                } else {
                    status_.is_live = false;
                    status_.viewers = 0;
                    status_.status_text = "Connected (Offline)";
                }
                status_.connected = true;
            } catch (...) {
                status_.connected = false;
                status_.status_text = "Parse error";
            }
        } else {
            status_.connected = false;
            status_.status_text = resp.error;
        }
    }

    // 2. Current channel title & game
    {
        std::string url = "https://api.twitch.tv/helix/channels?broadcaster_id=" + status_.broadcaster_id;
        auto resp = http::get(url, hdrs, 5000);
        if (resp.ok) {
            try {
                json j = json::parse(resp.body);
                if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
                    status_.current_title = j["data"][0].value("title", "");
                    status_.current_game = j["data"][0].value("game_name", "");
                }
            } catch (...) {
            }
        }
    }

    std::string out_str = fmt::format_twitch_viewers(
        status_.viewers, status_.is_live, settings.viewers_format, settings.offline_text);
    text_writer_.write_if_changed(out_dir / "TwitchViewers.txt", out_str);
}

std::vector<TwitchCustomGame> TwitchClient::search_categories(const TwitchSettings& settings,
                                                              const std::string& query) {
    std::vector<TwitchCustomGame> results;
    if (query.empty() || settings.client_id.empty() || settings.oauth_token.empty()) {
        return results;
    }
    std::string url = "https://api.twitch.tv/helix/search/categories?first=10&query=" + fmt::url_encode(query);
    auto resp = http::get(url, auth_headers(settings), 5000);
    if (!resp.ok) return results;
    try {
        json j = json::parse(resp.body);
        if (j.contains("data") && j["data"].is_array()) {
            for (const auto& item : j["data"]) {
                TwitchCustomGame g;
                g.id = item.value("id", "");
                g.name = item.value("name", "");
                if (!g.id.empty() && !g.name.empty()) results.push_back(std::move(g));
            }
        }
    } catch (...) {
    }
    return results;
}

bool TwitchClient::update_channel_info(const TwitchSettings& settings,
                                       const std::string& title,
                                       const std::string& game_id,
                                       std::string& out_error) {
    if (!ensure_broadcaster_id(settings)) {
        out_error = status_.status_text;
        return false;
    }
    json body = json::object();
    if (!title.empty()) body["title"] = title;
    if (!game_id.empty()) body["game_id"] = game_id;
    if (body.empty()) {
        out_error = "Nothing to update";
        return false;
    }

    std::string url = "https://api.twitch.tv/helix/channels?broadcaster_id=" + status_.broadcaster_id;
    auto resp = http::request("PATCH", url, auth_headers(settings), body.dump(), 5000);
    if (!resp.ok) {
        out_error = resp.error;
        return false;
    }
    if (!title.empty()) status_.current_title = title;
    return true;
}

} // namespace osmv::daemon

