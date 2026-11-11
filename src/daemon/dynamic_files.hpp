#pragma once

#include "common/types.hpp"
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace osmv::daemon {

class DynamicFilesRunner {
public:
    void sync_rules(const DynamicFilesSettings& settings, const std::filesystem::path& out_dir);
    void tick(const DynamicFilesSettings& settings, const std::filesystem::path& out_dir);
    std::vector<DynamicRuleStatus> statuses() const;

private:
    struct RuleRuntime {
        std::chrono::steady_clock::time_point next_run{};
        DynamicRuleStatus status;
    };
    std::vector<RuleRuntime> runtimes_;
    std::string last_config_sig_;
};

} // namespace osmv::daemon

