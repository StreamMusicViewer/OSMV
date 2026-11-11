#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osmv::os {

#ifdef _WIN32
std::wstring utf8_to_wide(const std::string& s);
std::string wide_to_utf8(const std::wstring& w);
#endif

std::filesystem::path path_from_utf8(const std::string& utf8);
std::string path_to_utf8(const std::filesystem::path& p);

std::filesystem::path exe_path();
std::filesystem::path exe_dir();
std::filesystem::path temp_dir();

void open_url_in_browser(const std::string& url);
void open_folder(const std::filesystem::path& dir);
bool spawn_detached(const std::filesystem::path& exe, const std::vector<std::string>& args);

bool is_autostart_enabled();
bool set_autostart(bool enabled);

// Returns current process resident/private working set memory in KiB
uint64_t get_process_memory_kb();

// Sleep helper
void sleep_ms(int ms);

class SingleInstanceGuard {
public:
    SingleInstanceGuard() = default;
    ~SingleInstanceGuard();
    SingleInstanceGuard(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;

    bool try_acquire(const std::string& app_id);
    void release();

private:
#ifdef _WIN32
    void* mutex_handle_ = nullptr;
#else
    int lock_fd_ = -1;
    std::string lock_path_;
#endif
};

} // namespace osmv::os

