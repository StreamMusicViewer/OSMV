#pragma once

#include "common/types.hpp"
#include <memory>

namespace osmv::daemon {

class SystemSampler {
public:
    virtual ~SystemSampler() = default;
    virtual SystemMetrics sample() = 0;
};

std::unique_ptr<SystemSampler> create_system_sampler();

} // namespace osmv::daemon

