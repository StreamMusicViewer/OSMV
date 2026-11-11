#include "discord_rpc.hpp"
#include "common/formatter.hpp"
#include "common/http.hpp"
#include "common/os_utils.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace osmv::daemon {

namespace {
using json = nlohmann::json;

bool is_numeric_id(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

std::string truncate_utf8(const std::string& s, size_t max_bytes) {
    if (s.size() <= max_bytes) return s;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t char_len = 1;
        if ((c & 0x80) == 0) char_len = 1;
        else if ((c & 0xE0) == 0xC0) char_len = 2;
        else if ((c & 0xF0) == 0xE0) char_len = 3;
        else if ((c & 0xF8) == 0xF0) char_len = 4;
        if (i + char_len > max_bytes) break;
        i += char_len;
    }
    return s.substr(0, i);
}

std::string pad_discord_text(std::string s) {
    if (s.size() == 1) s.push_back(' ');
    return truncate_utf8(s, 128);
}

class DiscordIpcTransport {
public:
    ~DiscordIpcTransport() { close_conn(); }

    bool is_open() const {
#ifdef _WIN32
        return pipe_ != INVALID_HANDLE_VALUE;
#else
        return fd_ >= 0;
#endif
    }

    bool check_alive() {
        if (!is_open()) return false;
#ifdef _WIN32
        DWORD avail = 0;
        if (!PeekNamedPipe(pipe_, nullptr, 0, nullptr, &avail, nullptr)) {
            close_conn();
            return false;
        }
        if (avail >= 8) {
            uint32_t op = 0;
            std::string msg;
            if (!read_frame(op, msg, 10) || op == 2) {
                close_conn();
                return false;
            }
        }
        return true;
#else
        pollfd pfd{fd_, POLLIN, 0};
        int rc = poll(&pfd, 1, 0);
        if (rc < 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            close_conn();
            return false;
        }
        if (pfd.revents & POLLIN) {
            char peek_buf[1];
            ssize_t n = recv(fd_, peek_buf, 1, MSG_PEEK | MSG_DONTWAIT);
            if (n == 0) {
                close_conn();
                return false;
            }
            uint32_t op = 0;
            std::string msg;
            if (!read_frame(op, msg, 10) || op == 2) {
                close_conn();
                return false;
            }
        }
        return true;
#endif
    }

    bool connect_and_handshake(const std::string& client_id) {
        close_conn();
        if (!open_any_pipe()) return false;

        json hs = {{"v", 1}, {"client_id", client_id}};
        if (!write_frame(0, hs.dump())) {
            close_conn();
            return false;
        }
        uint32_t op = 0;
        std::string resp;
        if (!read_frame(op, resp, 1500) || op == 2) {
            close_conn();
            return false;
        }
        return true;
    }

    enum class SendResult { Ok, ApiError, PipeClosed };

    SendResult send_activity(const json& payload) {
        if (!is_open()) return SendResult::PipeClosed;
        std::string serialized;
        try {
            serialized = payload.dump(-1, ' ', false, json::error_handler_t::replace);
        } catch (...) {
            return SendResult::ApiError;
        }
        if (!write_frame(1, serialized)) {
            close_conn();
            return SendResult::PipeClosed;
        }

        uint32_t op = 0;
        std::string reply;
        if (read_frame(op, reply, 600)) {
            if (op == 2) {
                close_conn();
                return SendResult::PipeClosed;
            }
            try {
                auto rj = json::parse(reply);
                if (rj.contains("evt") && rj["evt"].is_string() && rj["evt"] == "ERROR") {
                    return SendResult::ApiError;
                }
            } catch (...) {
            }
        }
        return SendResult::Ok;
    }

    void close_conn() {
#ifdef _WIN32
        if (pipe_ != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe_);
            pipe_ = INVALID_HANDLE_VALUE;
        }
#else
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
#endif
    }

private:
    bool open_any_pipe() {
#ifdef _WIN32
        for (int i = 0; i < 10; ++i) {
            std::wstring name = L"\\\\?\\pipe\\discord-ipc-" + std::to_wstring(i);
            HANDLE h = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                   OPEN_EXISTING, 0, nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                pipe_ = h;
                return true;
            }
        }
        return false;
#else
        std::vector<std::string> dirs;
        if (const char* xdg = std::getenv("XDG_RUNTIME_DIR"); xdg && xdg[0]) {
            dirs.emplace_back(xdg);
            dirs.emplace_back(std::string(xdg) + "/app/com.discordapp.Discord");
            dirs.emplace_back(std::string(xdg) + "/snap.discord");
        }
        dirs.emplace_back("/tmp");

        for (const auto& dir : dirs) {
            for (int i = 0; i < 10; ++i) {
                std::string p = dir + "/discord-ipc-" + std::to_string(i);
                int fd = socket(AF_UNIX, SOCK_STREAM, 0);
                if (fd < 0) continue;

                sockaddr_un addr{};
                addr.sun_family = AF_UNIX;
                if (p.size() < sizeof(addr.sun_path)) {
                    std::strncpy(addr.sun_path, p.c_str(), sizeof(addr.sun_path) - 1);
                    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
                        timeval tv{};
                        tv.tv_sec = 1;
                        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                        fd_ = fd;
                        return true;
                    }
                }
                close(fd);
            }
        }
        return false;
#endif
    }

    bool write_exact(const void* data, size_t len) {
#ifdef _WIN32
        const uint8_t* p = static_cast<const uint8_t*>(data);
        size_t sent = 0;
        while (sent < len) {
            DWORD n = 0;
            if (!WriteFile(pipe_, p + sent, static_cast<DWORD>(len - sent), &n, nullptr) || n == 0) {
                return false;
            }
            sent += n;
        }
        return true;
#else
        const uint8_t* p = static_cast<const uint8_t*>(data);
        size_t sent = 0;
        while (sent < len) {
            ssize_t n = send(fd_, p + sent, len - sent, MSG_NOSIGNAL);
            if (n <= 0) {
                if (n < 0 && errno == EINTR) continue;
                return false;
            }
            sent += static_cast<size_t>(n);
        }
        return true;
#endif
    }

    bool read_exact(void* data, size_t len) {
#ifdef _WIN32
        uint8_t* p = static_cast<uint8_t*>(data);
        size_t got = 0;
        while (got < len) {
            DWORD n = 0;
            if (!ReadFile(pipe_, p + got, static_cast<DWORD>(len - got), &n, nullptr) || n == 0) {
                return false;
            }
            got += n;
        }
        return true;
#else
        uint8_t* p = static_cast<uint8_t*>(data);
        size_t got = 0;
        while (got < len) {
            ssize_t n = recv(fd_, p + got, len - got, 0);
            if (n <= 0) {
                if (n < 0 && errno == EINTR) continue;
                return false;
            }
            got += static_cast<size_t>(n);
        }
        return true;
#endif
    }

    bool write_frame(uint32_t opcode, const std::string& payload) {
        uint32_t hdr[2] = {opcode, static_cast<uint32_t>(payload.size())};
        return write_exact(hdr, sizeof(hdr)) &&
               (payload.empty() || write_exact(payload.data(), payload.size()));
    }

    bool read_frame(uint32_t& opcode, std::string& out, int timeout_ms) {
#ifdef _WIN32
        auto start = std::chrono::steady_clock::now();
        for (;;) {
            DWORD avail = 0;
            if (!PeekNamedPipe(pipe_, nullptr, 0, nullptr, &avail, nullptr)) return false;
            if (avail >= 8) break;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start).count() >= timeout_ms) {
                return false;
            }
            os::sleep_ms(15);
        }
#else
        pollfd pfd{fd_, POLLIN, 0};
        if (poll(&pfd, 1, timeout_ms) <= 0) return false;
#endif
        uint32_t hdr[2] = {0, 0};
        if (!read_exact(hdr, sizeof(hdr))) return false;
        opcode = hdr[0];
        uint32_t len = hdr[1];
        if (len > 256 * 1024) return false;
        out.resize(len);
        return len == 0 || read_exact(out.data(), len);
    }

#ifdef _WIN32
    HANDLE pipe_ = INVALID_HANDLE_VALUE;
#else
    int fd_ = -1;
#endif
};

uint32_t current_pid() {
#ifdef _WIN32
    return static_cast<uint32_t>(GetCurrentProcessId());
#else
    return static_cast<uint32_t>(getpid());
#endif
}

std::string to_lower_ascii(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string strip_topic_suffix(std::string artist) {
    constexpr const char* kTopic = " - Topic";
    if (artist.size() > 8 && artist.compare(artist.size() - 8, 8, kTopic) == 0) {
        artist.resize(artist.size() - 8);
    }
    return artist;
}

std::string fetch_itunes_cover(const std::string& title, const std::string& artist) {
    std::string clean_artist = strip_topic_suffix(artist);
    std::string raw = title + " " + clean_artist;
    std::string clean;
    clean.reserve(raw.size());
    bool last_space = true;
    for (unsigned char c : raw) {
        if (std::isalnum(c)) {
            clean.push_back(static_cast<char>(c));
            last_space = false;
        } else if (!last_space) {
            clean.push_back('+');
            last_space = true;
        }
    }
    if (!clean.empty() && clean.back() == '+') clean.pop_back();
    if (clean.empty()) return {};

    std::string url = "https://itunes.apple.com/search?term=" + clean + "&entity=song&limit=1&media=music";
    auto resp = http::get(url, {}, 3500);
    if (!resp.ok || resp.body.empty()) return {};

    try {
        json j = json::parse(resp.body);
        if (j.contains("results") && j["results"].is_array() && !j["results"].empty()) {
            const auto& first = j["results"][0];
            if (!clean_artist.empty() && clean_artist != "Unknown Artist" &&
                first.contains("artistName") && first["artistName"].is_string()) {
                std::string a_low = to_lower_ascii(clean_artist);
                std::string r_low = to_lower_ascii(first["artistName"].get<std::string>());
                if (r_low.find(a_low) == std::string::npos && a_low.find(r_low) == std::string::npos) {
                    return {};
                }
            }
            if (first.contains("artworkUrl100") && first["artworkUrl100"].is_string()) {
                return fmt::replace_all(first["artworkUrl100"].get<std::string>(), "100x100bb", "500x500bb");
            }
        }
    } catch (...) {
    }
    return {};
}
} // namespace

DiscordRpcManager::~DiscordRpcManager() {
    stop();
}

void DiscordRpcManager::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread(&DiscordRpcManager::run_loop, this);
}

void DiscordRpcManager::stop() {
    if (!running_.exchange(false)) return;
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

void DiscordRpcManager::post_update(const DiscordSettings& settings,
                                    const std::optional<std::string>& title,
                                    const std::optional<std::string>& artist,
                                    const std::optional<std::string>& album,
                                    const std::optional<std::string>& art_url,
                                    const std::string& status) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_ = Command{settings, title, artist, album, art_url, status};
        has_pending_ = true;
    }
    cv_.notify_one();
}

std::string DiscordRpcManager::status_string() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return status_;
}

std::string DiscordRpcManager::resolve_cover_url(const std::string& title,
                                                 const std::string& artist,
                                                 const std::optional<std::string>& art_url,
                                                 const std::string& fallback_key) {
    std::string key = title + "|" + artist;
    if (auto it = cache_map_.find(key); it != cache_map_.end()) {
        cache_lru_.splice(cache_lru_.begin(), cache_lru_, it->second);
        return it->second->second;
    }

    std::string resolved = fetch_itunes_cover(title, artist);
    if (resolved.empty() && art_url.has_value() && art_url->rfind("https://", 0) == 0) {
        resolved = *art_url;
    }
    if (resolved.empty()) {
        resolved = fallback_key.empty() ? "osmv_logo" : fallback_key;
    }

    constexpr size_t kMaxCacheEntries = 32;
    if (cache_lru_.size() >= kMaxCacheEntries) {
        auto last = cache_lru_.back().first;
        cache_map_.erase(last);
        cache_lru_.pop_back();
    }
    cache_lru_.emplace_front(key, resolved);
    cache_map_[key] = cache_lru_.begin();
    return resolved;
}

void DiscordRpcManager::run_loop() {
    DiscordIpcTransport ipc;
    std::string current_client_id;
    std::string last_sent_json;
    uint64_t nonce_counter = 1;
    auto next_reconnect = std::chrono::steady_clock::time_point{};

    auto set_status = [&](const std::string& st) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        status_ = st;
    };

    while (running_.load()) {
        Command cmd;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(2), [&] {
                return !running_.load() || has_pending_;
            });
            if (!running_.load()) break;
            if (!has_pending_) continue;
            cmd = std::move(pending_);
            has_pending_ = false;
        }

        const auto& s = cmd.settings;
        if (!s.enabled) {
            if (ipc.is_open()) {
                json clr = {
                    {"cmd", "SET_ACTIVITY"},
                    {"args", {{"pid", current_pid()}}},
                    {"nonce", std::to_string(nonce_counter++)}
                };
                ipc.send_activity(clr);
                ipc.close_conn();
            }
            current_client_id.clear();
            last_sent_json.clear();
            cache_lru_.clear();
            cache_map_.clear();
            set_status("disabled");
            continue;
        }

        if (!is_numeric_id(s.client_id)) {
            ipc.close_conn();
            current_client_id.clear();
            last_sent_json.clear();
            set_status("invalid_id");
            continue;
        }

        if (s.client_id != current_client_id || !ipc.check_alive()) {
            auto now = std::chrono::steady_clock::now();
            if (s.client_id != current_client_id) {
                ipc.close_conn();
                current_client_id = s.client_id;
                last_sent_json.clear();
                next_reconnect = std::chrono::steady_clock::time_point{};
            }
            if (now < next_reconnect) {
                continue;
            }
            set_status("connecting");
            if (!ipc.connect_and_handshake(s.client_id)) {
                set_status("waiting_discord");
                next_reconnect = now + std::chrono::seconds(5);
                continue;
            }
            set_status("connected");
            last_sent_json.clear();
        }

        const bool music_active = cmd.title.has_value() && s.use_music_when_playing;
        std::string artist_str = cmd.artist.value_or("");
        std::string album_str = cmd.album.value_or("");
        std::string details = music_active ? *cmd.title : s.custom_details;
        std::string state;
        if (music_active) {
            if (!artist_str.empty() && artist_str != "Unknown Artist") {
                state = "by " + artist_str;
            }
        } else {
            state = s.custom_state;
        }

        std::string large_image;
        std::string large_text;
        if (music_active) {
            large_image = resolve_cover_url(*cmd.title, artist_str, cmd.art_url, s.large_image_key);
            // In Discord type=2 (Listening), large_text is rendered as line 3 under state.
            // Only populate it with the album name (never repeat artist/title).
            if (!album_str.empty() && album_str != artist_str && album_str != *cmd.title) {
                large_text = album_str;
            }
        } else {
            large_image = s.large_image_key.empty() ? "osmv_logo" : s.large_image_key;
            large_text = s.large_image_text;
        }

        std::string small_key;
        std::string small_text;
        if (music_active) {
            if (cmd.status == "playing") {
                small_key = s.status_key_playing;
                small_text = "Playing";
            } else if (cmd.status == "paused") {
                small_key = s.status_key_paused;
                small_text = "Paused";
            } else if (cmd.status == "stopped") {
                small_key = s.status_key_stopped;
                small_text = "Stopped";
            }
        } else {
            small_key = s.small_image_key;
            small_text = s.small_image_text;
        }

        json activity = json::object();
        activity["type"] = music_active ? 2 : 0; // 2 = Listening to, 0 = Playing
        if (music_active) {
            activity["status_display_type"] = 1; // Show track title in status line
        }
        if (!details.empty()) activity["details"] = pad_discord_text(details);
        if (!state.empty()) activity["state"] = pad_discord_text(state);

        json assets = json::object();
        if (!large_image.empty()) assets["large_image"] = large_image;
        if (!large_text.empty()) assets["large_text"] = pad_discord_text(large_text);
        if (!small_key.empty()) {
            assets["small_image"] = small_key;
            if (!small_text.empty()) assets["small_text"] = pad_discord_text(small_text);
        }
        if (!assets.empty()) activity["assets"] = std::move(assets);

        std::string activity_sig = activity.dump(-1, ' ', false, json::error_handler_t::replace);
        if (activity_sig == last_sent_json) {
            continue;
        }

        json payload = {
            {"cmd", "SET_ACTIVITY"},
            {"args", {
                {"pid", current_pid()},
                {"activity", std::move(activity)}
            }},
            {"nonce", std::to_string(nonce_counter++)}
        };

        auto now_send = std::chrono::steady_clock::now();
        if (now_send < next_reconnect) {
            continue;
        }

        auto res = ipc.send_activity(payload);
        if (res == DiscordIpcTransport::SendResult::Ok) {
            last_sent_json = std::move(activity_sig);
            set_status("connected");
        } else if (res == DiscordIpcTransport::SendResult::ApiError) {
            set_status("error");
            last_sent_json.clear();
            next_reconnect = now_send + std::chrono::seconds(5);
        } else {
            set_status("waiting_discord");
            last_sent_json.clear();
            next_reconnect = now_send + std::chrono::seconds(5);
        }
    }

    if (ipc.is_open()) {
        json clr = {
            {"cmd", "SET_ACTIVITY"},
            {"args", {{"pid", current_pid()}}},
            {"nonce", std::to_string(nonce_counter++)}
        };
        ipc.send_activity(clr);
        ipc.close_conn();
    }
}

} // namespace osmv::daemon

