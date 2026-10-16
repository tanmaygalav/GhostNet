#pragma once

#include "ghostnet/event.hpp"
#include <cstdint>
#include <string>

namespace ghostnet {

enum class FaultType : uint8_t {
    SERVICE_OUTAGE,
    LATENCY_SPIKE
};

struct FaultConfig {
    uint32_t id{0};
    FaultType type{FaultType::SERVICE_OUTAGE};
    ComponentId target_component_id{0}; // Node ID for OUTAGE, Link ID for LATENCY_SPIKE
    SimTime start_time_us{0};
    SimTime duration_us{0};
    
    // For LATENCY_SPIKE: additional latency in microseconds
    SimTime extra_latency_us{0};
};

} // namespace ghostnet