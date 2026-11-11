#pragma once

#include "common/types.hpp"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace osmv::daemon {

class DiscordRpcManager {
public:
    DiscordRpcManager() = default;
    ~DiscordRpcManager();
    DiscordRpcManager(const DiscordRpcManager&) = delete;
    DiscordRpcManager& operator=(const DiscordRpcManager&) = delete;

    void start();
    void stop();

    void post_update(const DiscordSettings& settings,
                     const std::optional<std::string>& title,
                     const std::optional<std::string>& artist,
                     const std::optional<std::string>& art_url,
                     const std::string& status);

    std::string status_string() const;

private:
    struct Command {
        DiscordSettings settings;
        std::optional<std::string> title;
        std::optional<std::string> artist;
        std::optional<std::string> art_url;
        std::string status;
    };

    void run_loop();
    std::string resolve_cover_url(const std::string& title,
                                  const std::string& artist,
                                  const std::optional<std::string>& art_url,
                                  const std::string& fallback_key);

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool has_pending_ = false;
    Command pending_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    mutable std::mutex status_mutex_;
    std::string status_ = "disabled";

    // Bounded cover art cache (max 32 entries)
    std::list<std::pair<std::string, std::string>> cache_lru_;
    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> cache_map_;
};

} // namespace osmv::daemon

