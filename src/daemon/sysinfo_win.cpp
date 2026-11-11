#include "sysinfo.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>

#include <algorithm>
#include <chrono>
#include <vector>

namespace osmv::daemon {

namespace {
uint64_t filetime_to_u64(const FILETIME& ft) {
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

class WindowsSystemSampler final : public SystemSampler {
public:
    SystemMetrics sample() override {
        SystemMetrics m;

        // CPU usage via GetSystemTimes delta
        FILETIME ft_idle{}, ft_kernel{}, ft_user{};
        if (GetSystemTimes(&ft_idle, &ft_kernel, &ft_user)) {
            uint64_t idle = filetime_to_u64(ft_idle);
            uint64_t kernel = filetime_to_u64(ft_kernel);
            uint64_t user = filetime_to_u64(ft_user);

            if (has_prev_cpu_) {
                uint64_t d_idle = idle - prev_idle_;
                uint64_t d_total = (kernel - prev_kernel_) + (user - prev_user_);
                if (d_total > 0 && d_total >= d_idle) {
                    m.cpu_percent = std::clamp(
                        100.0 * static_cast<double>(d_total - d_idle) / static_cast<double>(d_total),
                        0.0, 100.0);
                }
            }
            prev_idle_ = idle;
            prev_kernel_ = kernel;
            prev_user_ = user;
            has_prev_cpu_ = true;
        }

        // RAM via GlobalMemoryStatusEx
        MEMORYSTATUSEX mem{};
        mem.dwLength = sizeof(mem);
        if (GlobalMemoryStatusEx(&mem) && mem.ullTotalPhys > 0) {
            m.ram_total_mb = mem.ullTotalPhys / (1024ULL * 1024ULL);
            m.ram_avail_mb = mem.ullAvailPhys / (1024ULL * 1024ULL);
            double avail_pct = 100.0 * static_cast<double>(mem.ullAvailPhys) / static_cast<double>(mem.ullTotalPhys);
            m.ram_avail_percent = std::clamp(avail_pct, 0.0, 100.0);
            m.ram_in_use_percent = std::clamp(100.0 - m.ram_avail_percent, 0.0, 100.0);
        }

        // Process count via EnumProcesses
        DWORD pids[4096];
        DWORD bytes_returned = 0;
        if (EnumProcesses(pids, sizeof(pids), &bytes_returned)) {
            m.process_count = bytes_returned / sizeof(DWORD);
        }

        // Network RX/TX via GetIfTable2
        PMIB_IF_TABLE2 table = nullptr;
        if (GetIfTable2(&table) == NO_ERROR && table) {
            uint64_t rx_total = 0;
            uint64_t tx_total = 0;
            for (ULONG i = 0; i < table->NumEntries; ++i) {
                const MIB_IF_ROW2& row = table->Table[i];
                if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
                if (row.OperStatus != IfOperStatusUp) continue;
                if (row.InterfaceAndOperStatusFlags.FilterInterface) continue;
                if (!row.InterfaceAndOperStatusFlags.HardwareInterface) continue;
                rx_total += row.InOctets;
                tx_total += row.OutOctets;
            }
            FreeMibTable(table);

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
    uint64_t prev_idle_ = 0;
    uint64_t prev_kernel_ = 0;
    uint64_t prev_user_ = 0;

    bool has_prev_net_ = false;
    uint64_t prev_rx_ = 0;
    uint64_t prev_tx_ = 0;
    std::chrono::steady_clock::time_point prev_net_time_{};
};
} // namespace

std::unique_ptr<SystemSampler> create_system_sampler() {
    return std::make_unique<WindowsSystemSampler>();
}

} // namespace osmv::daemon

