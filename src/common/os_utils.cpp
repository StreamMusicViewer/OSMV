#include "os_utils.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#else
#include <fcntl.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach/mach.h>
#endif
#endif

namespace osmv::os {

#ifdef _WIN32
std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) return {};
    std::wstring w(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), len);
    return w;
}

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), len, nullptr, nullptr);
    return s;
}
#endif

std::filesystem::path path_from_utf8(const std::string& utf8) {
#ifdef _WIN32
    return std::filesystem::path(utf8_to_wide(utf8));
#else
    return std::filesystem::path(utf8);
#endif
}

std::string path_to_utf8(const std::filesystem::path& p) {
#ifdef _WIN32
    return wide_to_utf8(p.wstring());
#else
    return p.string();
#endif
}

std::filesystem::path exe_path() {
#ifdef _WIN32
    std::wstring buf(MAX_PATH, L'\0');
    for (;;) {
        DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (len == 0) return {};
        if (len < buf.size() - 1) {
            buf.resize(len);
            return std::filesystem::path(buf);
        }
        buf.resize(buf.size() * 2);
    }
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        std::error_code ec;
        return std::filesystem::canonical(buf, ec);
    }
    return {};
#else
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return std::filesystem::path(buf);
    }
    return {};
#endif
}

std::filesystem::path exe_dir() {
    auto p = exe_path();
    if (p.has_parent_path()) return p.parent_path();
    return std::filesystem::current_path();
}

std::filesystem::path temp_dir() {
    std::error_code ec;
    auto p = std::filesystem::temp_directory_path(ec);
    if (!ec && !p.empty()) return p;
#ifdef _WIN32
    return std::filesystem::path(L".");
#else
    return std::filesystem::path("/tmp");
#endif
}

void open_url_in_browser(const std::string& url) {
    if (url.empty()) return;
#ifdef _WIN32
    std::wstring wurl = utf8_to_wide(url);
    ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp("open", "open", url.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
#else
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp("xdg-open", "xdg-open", url.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
#endif
}

void open_folder(const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    open_url_in_browser(dir.string());
#endif
}

bool spawn_detached(const std::filesystem::path& exe, const std::vector<std::string>& args) {
#ifdef _WIN32
    std::wstring cmd = L"\"" + exe.wstring() + L"\"";
    for (const auto& a : args) {
        cmd += L" ";
        if (a.find(' ') != std::string::npos) {
            cmd += L"\"" + utf8_to_wide(a) + L"\"";
        } else {
            cmd += utf8_to_wide(a);
        }
    }
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe.wstring().c_str(), cmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW | DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        setsid();
        // Double fork to prevent zombies
        pid_t pid2 = fork();
        if (pid2 > 0) _exit(0);
        std::string exe_str = exe.string();
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(exe_str.c_str()));
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);
        execv(exe_str.c_str(), argv.data());
        _exit(127);
    }
    waitpid(pid, nullptr, 0);
    return true;
#endif
}

bool is_autostart_enabled() {
#ifdef _WIN32
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    DWORD type = 0;
    DWORD size = 0;
    bool exists = (RegQueryValueExW(hKey, L"OSMV", nullptr, &type, nullptr, &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return exists;
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (!home) return false;
    std::filesystem::path plist = std::filesystem::path(home) / "Library/LaunchAgents/io.github.streammusicviewer.osmv.plist";
    std::error_code ec;
    return std::filesystem::exists(plist, ec);
#else
    const char* home = std::getenv("HOME");
    if (!home) return false;
    std::filesystem::path desktop = std::filesystem::path(home) / ".config/autostart/osmv.desktop";
    std::error_code ec;
    return std::filesystem::exists(desktop, ec);
#endif
}

bool set_autostart(bool enabled) {
#ifdef _WIN32
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    bool ok = false;
    if (enabled) {
        std::wstring cmd = L"\"" + exe_path().wstring() + L"\" --silent";
        ok = (RegSetValueExW(hKey, L"OSMV", 0, REG_SZ,
                             reinterpret_cast<const BYTE*>(cmd.c_str()),
                             static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS);
    } else {
        LSTATUS st = RegDeleteValueW(hKey, L"OSMV");
        ok = (st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND);
    }
    RegCloseKey(hKey);
    return ok;
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (!home) return false;
    std::filesystem::path dir = std::filesystem::path(home) / "Library/LaunchAgents";
    std::filesystem::path plist = dir / "io.github.streammusicviewer.osmv.plist";
    std::error_code ec;
    if (!enabled) {
        std::filesystem::remove(plist, ec);
        return true;
    }
    std::filesystem::create_directories(dir, ec);
    std::ofstream f(plist, std::ios::trunc);
    if (!f) return false;
    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
         "<plist version=\"1.0\">\n"
         "<dict>\n"
         "    <key>Label</key>\n"
         "    <string>io.github.streammusicviewer.osmv</string>\n"
         "    <key>ProgramArguments</key>\n"
         "    <array>\n"
         "        <string>" << exe_path().string() << "</string>\n"
         "        <string>--silent</string>\n"
         "    </array>\n"
         "    <key>RunAtLoad</key>\n"
         "    <true/>\n"
         "</dict>\n"
         "</plist>\n";
    return true;
#else
    const char* home = std::getenv("HOME");
    if (!home) return false;
    std::filesystem::path dir = std::filesystem::path(home) / ".config/autostart";
    std::filesystem::path desktop = dir / "osmv.desktop";
    std::error_code ec;
    if (!enabled) {
        std::filesystem::remove(desktop, ec);
        return true;
    }
    std::filesystem::create_directories(dir, ec);
    std::ofstream f(desktop, std::ios::trunc);
    if (!f) return false;
    f << "[Desktop Entry]\n"
         "Type=Application\n"
         "Name=OSMV\n"
         "Comment=OBS Stream Music Viewer Daemon\n"
         "Exec=\"" << exe_path().string() << "\" --silent\n"
         "Terminal=false\n"
         "X-GNOME-Autostart-enabled=true\n";
    return true;
#endif
}

uint64_t get_process_memory_kb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        return static_cast<uint64_t>(pmc.PrivateUsage / 1024);
    }
    return 0;
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        return static_cast<uint64_t>(info.resident_size / 1024);
    }
    return 0;
#else
    // Read VmRSS or RssAnon from /proc/self/status for clean RSS in KiB
    std::ifstream f("/proc/self/status");
    std::string line;
    uint64_t rss_kb = 0;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            rss_kb = std::strtoull(line.c_str() + 6, nullptr, 10);
            break;
        }
    }
    return rss_kb;
#endif
}

void sleep_ms(int ms) {
    if (ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

SingleInstanceGuard::~SingleInstanceGuard() {
    release();
}

bool SingleInstanceGuard::try_acquire(const std::string& app_id) {
#ifdef _WIN32
    std::wstring name = L"Local\\" + utf8_to_wide(app_id);
    HANDLE h = CreateMutexW(nullptr, TRUE, name.c_str());
    if (!h) return false;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(h);
        return false;
    }
    mutex_handle_ = h;
    return true;
#else
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    std::string base = (xdg && xdg[0]) ? std::string(xdg) : std::string("/tmp");
    lock_path_ = base + "/" + app_id + ".lock";
    int fd = open(lock_path_.c_str(), O_CREAT | O_RDWR, 0600);
    if (fd < 0) return false;
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return false;
    }
    lock_fd_ = fd;
    return true;
#endif
}

void SingleInstanceGuard::release() {
#ifdef _WIN32
    if (mutex_handle_) {
        HANDLE h = static_cast<HANDLE>(mutex_handle_);
        ReleaseMutex(h);
        CloseHandle(h);
        mutex_handle_ = nullptr;
    }
#else
    if (lock_fd_ >= 0) {
        flock(lock_fd_, LOCK_UN);
        close(lock_fd_);
        lock_fd_ = -1;
    }
#endif
}

} // namespace osmv::os
