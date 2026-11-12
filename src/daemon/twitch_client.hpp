#pragma once

#include "common/settings.hpp"
#include "common/types.hpp"
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace osmv::daemon {

class TwitchClient {
public:
    void tick(const TwitchSettings& settings, const std::filesystem::path& out_dir);
    TwitchStatus status() const { return status_; }

    // Interactive commands invoked by GUI over IPC
    std::vector<TwitchCustomGame> search_categories(const TwitchSettings& settings,
                                                    const std::string& query);
    bool update_channel_info(const TwitchSettings& settings,
                             const std::string& title,
                             const std::string& game_id,
                             std::string& out_error);

private:
    bool ensure_broadcaster_id(const TwitchSettings& settings);
    std::vector<std::pair<std::string, std::string>> auth_headers(const TwitchSettings& settings) const;

    TwitchStatus status_;
    cfg::CachedTextWriter text_writer_;
    std::string cached_channel_;
    std::string cached_client_id_;
    std::string cached_token_;
    std::chrono::steady_clock::time_point next_poll_{};
};

} // namespace osmv::daemon

