#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace osmv::gui {

class TwitchOAuthListener {
public:
    TwitchOAuthListener() = default;
    ~TwitchOAuthListener();
    TwitchOAuthListener(const TwitchOAuthListener&) = delete;
    TwitchOAuthListener& operator=(const TwitchOAuthListener&) = delete;

    // Starts listening on loopback port 80 (127.0.0.1 and ::1).
    // Returns false if port 80 could not be bound.
    bool start(const std::string& lang = "fr");
    void stop();

    bool is_listening() const { return listening_.load(); }

    // Returns true once when a token or error result is ready.
    bool poll_result(std::string& out_token, std::string& out_error);

private:
    void run_loop();

    std::atomic<bool> running_{false};
    std::atomic<bool> listening_{false};
    std::string lang_ = "fr";
    std::thread thread_;

    std::mutex res_mutex_;
    bool has_result_ = false;
    std::string result_token_;
    std::string result_error_;

#ifdef _WIN32
    uintptr_t sock4_ = ~uintptr_t(0);
    uintptr_t sock6_ = ~uintptr_t(0);
#else
    int sock4_ = -1;
    int sock6_ = -1;
#endif
};

} // namespace osmv::gui
