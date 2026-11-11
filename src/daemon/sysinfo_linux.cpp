#include "sysinfo.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>

namespace osmv::daemon {

namespace {
bool is_digits(const char* s) {
    if (!s || !*s) return false;
    for (; *s; ++s) {
        if (!std::isdigit(static_cast<unsigned char>(*s))) return false;
    }
    return true;
}

class LinuxSystemSampler final : public SystemSampler {
public:
    SystemMetrics sample() override {
        SystemMetrics m;

        // CPU via /proc/stat
        {
            std::ifstream f("/proc/stat");
            std::string label;
            uint64_t u = 0, n = 0, s = 0, idle = 0, iow = 0, irq = 0, sirq = 0, steal = 0;
            if (f >> label >> u >> n >> s >> idle >> iow >> irq >> sirq >> steal && label == "cpu") {
                uint64_t idle_all = idle + iow;
                uint64_t total = u + n + s + idle + iow + irq + sirq + steal;
                if (has_prev_cpu_) {
                    uint64_t d_total = total - prev_total_;
                    uint64_t d_idle = idle_all - prev_idle_;
                    if (d_total > 0 && d_total >= d_idle) {
                        m.cpu_percent = std::clamp(
                            100.0 * static_cast<double>(d_total - d_idle) / static_cast<double>(d_total),
                            0.0, 100.0);
                    }
                }
                prev_total_ = total;
                prev_idle_ = idle_all;
                has_prev_cpu_ = true;
            }
        }

        // RAM via /proc/meminfo
        {
            std::ifstream f("/proc/meminfo");
            std::string line;
            uint64_t mem_total_kb = 0;
            uint64_t mem_avail_kb = 0;
            uint64_t mem_free_kb = 0;
            while (std::getline(f, line)) {
                if (line.rfind("MemTotal:", 0) == 0) {
                    std::sscanf(line.c_str() + 9, "%llu", reinterpret_cast<unsigned long long*>(&mem_total_kb));
                } else if (line.rfind("MemAvailable:", 0) == 0) {
                    std::sscanf(line.c_str() + 13, "%llu", reinterpret_cast<unsigned long long*>(&mem_avail_kb));
                } else if (line.rfind("MemFree:", 0) == 0) {
                    std::sscanf(line.c_str() + 8, "%llu", reinterpret_cast<unsigned long long*>(&mem_free_kb));
                }
            }
            if (mem_avail_kb == 0) mem_avail_kb = mem_free_kb;
            if (mem_total_kb > 0) {
                m.ram_total_mb = mem_total_kb / 1024ULL;
                m.ram_avail_mb = mem_avail_kb / 1024ULL;
                double avail_pct = 100.0 * static_cast<double>(mem_avail_kb) / static_cast<double>(mem_total_kb);
                m.ram_avail_percent = std::clamp(avail_pct, 0.0, 100.0);
                m.ram_in_use_percent = std::clamp(100.0 - m.ram_avail_percent, 0.0, 100.0);
            }
        }

        // Process count via /proc/<pid>
        {
            if (DIR* dir = opendir("/proc")) {
                uint32_t count = 0;
                while (dirent* ent = readdir(dir)) {
                    if (is_digits(ent->d_name)) ++count;
                }
                closedir(dir);
                m.process_count = count;
            }
        }

        // Network RX/TX via /proc/net/dev
        {
            std::ifstream f("/proc/net/dev");
            std::string line;
            std::getline(f, line);
            std::getline(f, line);
            uint64_t rx_total = 0;
            uint64_t tx_total = 0;
            while (std::getline(f, line)) {
                auto colon = line.find(':');
                if (colon == std::string::npos) continue;
                std::string iface = line.substr(0, colon);
                size_t first = iface.find_first_not_of(" \t");
                if (first != std::string::npos) iface = iface.substr(first);
                if (iface == "lo") continue;

                std::istringstream iss(line.substr(colon + 1));
                uint64_t rx_bytes = 0, dummy = 0, tx_bytes = 0;
                if (iss >> rx_bytes >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> tx_bytes) {
                    rx_total += rx_bytes;
                    tx_total += tx_bytes;
                }
            }

            auto now = std::chrono::steady_clock::now();
            if (has_prev_net_) {
                double dt = std::chrono::duration<double>(now - prev_net_time_).count();
                if (dt > 0.05) {
                    if (rx_total >= prev_rx_) {
                        m.download_bytes_per_sec = static_cast<double>(rx_total - prev_rx_) / dt;
                    }
                    if (tx_total >= prev_tx_) {
                        m.upload_bytes_per_sec = static_cast<double>(tx_total - prev_tx_) / dt;
                    }
                }
            }
            prev_rx_ = rx_total;
            prev_tx_ = tx_total;
            prev_net_time_ = now;
            has_prev_net_ = true;
        }

        return m;
    }

private:
    bool has_prev_cpu_ = false;
    uint64_t prev_total_ = 0;
    uint64_t prev_idle_ = 0;

    bool has_prev_net_ = false;
    uint64_t prev_rx_ = 0;
    uint64_t prev_tx_ = 0;
    std::chrono::steady_clock::time_point prev_net_time_{};
};
} // namespace

std::unique_ptr<SystemSampler> create_system_sampler() {
    return std::make_unique<LinuxSystemSampler>();
}

} // namespace osmv::daemon

