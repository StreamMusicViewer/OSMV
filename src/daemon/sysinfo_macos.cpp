#include "sysinfo.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_dl.h>

namespace osmv::daemon {

class MacOSSystemSampler final : public SystemSampler {
public:
    SystemMetrics sample() override {
        SystemMetrics m;

        // 1. CPU Usage via host_cpu_load_info
        {
            host_cpu_load_info_data_t cpu_info;
            mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
            if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                                reinterpret_cast<host_info_t>(&cpu_info), &count) == KERN_SUCCESS) {
                uint64_t user = cpu_info.cpu_ticks[CPU_STATE_USER];
                uint64_t sys = cpu_info.cpu_ticks[CPU_STATE_SYSTEM];
                uint64_t nice = cpu_info.cpu_ticks[CPU_STATE_NICE];
                uint64_t idle = cpu_info.cpu_ticks[CPU_STATE_IDLE];
                uint64_t total = user + sys + nice + idle;

                if (has_prev_cpu_) {
                    uint64_t d_total = total - prev_total_;
                    uint64_t d_idle = idle - prev_idle_;
                    if (d_total > 0 && d_total >= d_idle) {
                        m.cpu_percent = std::clamp(
                            100.0 * static_cast<double>(d_total - d_idle) / static_cast<double>(d_total),
                            0.0, 100.0);
                    }
                }
                prev_total_ = total;
                prev_idle_ = idle;
                has_prev_cpu_ = true;
            }
        }

        // 2. RAM Usage via sysctl & host_statistics64
        {
            int mib[2] = {CTL_HW, HW_MEMSIZE};
            uint64_t mem_total_bytes = 0;
            size_t len = sizeof(mem_total_bytes);
            if (sysctl(mib, 2, &mem_total_bytes, &len, nullptr, 0) == 0 && mem_total_bytes > 0) {
                m.ram_total_mb = mem_total_bytes / (1024ULL * 1024ULL);
            }

            vm_size_t page_size = 4096;
            mach_port_t port = mach_host_self();
            _host_page_size(port, &page_size);

            vm_statistics64_data_t vm_stat;
            mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(natural_t);
            if (host_statistics64(port, HOST_VM_INFO64,
                                  reinterpret_cast<host_info64_t>(&vm_stat), &count) == KERN_SUCCESS) {
                uint64_t free_bytes = (static_cast<uint64_t>(vm_stat.free_count) +
                                       static_cast<uint64_t>(vm_stat.inactive_count)) * page_size;
                m.ram_avail_mb = free_bytes / (1024ULL * 1024ULL);
                if (m.ram_total_mb > 0) {
                    m.ram_avail_percent = std::clamp(
                        100.0 * static_cast<double>(m.ram_avail_mb) / static_cast<double>(m.ram_total_mb),
                        0.0, 100.0);
                    m.ram_in_use_percent = 100.0 - m.ram_avail_percent;
                }
            }
        }

        // 3. Process count via sysctl KERN_PROC
        {
            int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
            size_t proc_len = 0;
            if (sysctl(mib, 4, nullptr, &proc_len, nullptr, 0) == 0 && proc_len > 0) {
                m.process_count = static_cast<uint32_t>(proc_len / sizeof(struct kinfo_proc));
            }
        }

        // 4. Network throughput via getifaddrs
        {
            struct ifaddrs* ifap = nullptr;
            if (getifaddrs(&ifap) == 0 && ifap) {
                uint64_t rx_bytes = 0;
                uint64_t tx_bytes = 0;
                for (struct ifaddrs* cur = ifap; cur != nullptr; cur = cur->ifa_next) {
                    if (cur->ifa_addr && cur->ifa_addr->sa_family == AF_LINK) {
                        auto* data = reinterpret_cast<struct if_data*>(cur->ifa_data);
                        if (data && !(cur->ifa_flags & IFF_LOOPBACK) && (cur->ifa_flags & IFF_UP)) {
                            rx_bytes += data->ifi_ibytes;
                            tx_bytes += data->ifi_obytes;
                        }
                    }
                }
                freeifaddrs(ifap);

                auto now = std::chrono::steady_clock::now();
                if (has_prev_net_) {
                    double dt = std::chrono::duration<double>(now - prev_net_time_).count();
                    if (dt > 0.05) {
                        if (rx_bytes >= prev_rx_) {
                            m.download_bytes_per_sec = static_cast<double>(rx_bytes - prev_rx_) / dt;
                        }
                        if (tx_bytes >= prev_tx_) {
                            m.upload_bytes_per_sec = static_cast<double>(tx_bytes - prev_tx_) / dt;
                        }
                    }
                }
                prev_rx_ = rx_bytes;
                prev_tx_ = tx_bytes;
                prev_net_time_ = now;
                has_prev_net_ = true;
            }
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

std::unique_ptr<SystemSampler> create_system_sampler() {
    return std::make_unique<MacOSSystemSampler>();
}

} // namespace osmv::daemon
