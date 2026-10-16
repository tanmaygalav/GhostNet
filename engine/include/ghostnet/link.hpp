#pragma once

#include "ghostnet/event.hpp"
#include "ghostnet/node.hpp"
#include <cstdint>

namespace ghostnet {

using LinkId = uint32_t;

struct LinkConfig {
    LinkId id{0};
    ComponentId source_id{0};
    ComponentId target_id{0};
    
    SimTime propagation_delay{0};  // Latency in microseconds
    uint64_t bandwidth_bps{0};     // 0 means unbounded
    double loss_probability{0.0};  // [0.0, 1.0]
    uint32_t queue_capacity{1000};
    
    ComponentState state{ComponentState::UP};
};

class Link {
public:
    explicit Link(LinkConfig config) : config_(std::move(config)) {}

    [[nodiscard]] LinkId id() const noexcept { return config_.id; }
    [[nodiscard]] ComponentId source_id() const noexcept { return config_.source_id; }
    [[nodiscard]] ComponentId target_id() const noexcept { return config_.target_id; }
    [[nodiscard]] SimTime propagation_delay() const noexcept { return config_.propagation_delay; }
    [[nodiscard]] uint64_t bandwidth_bps() const noexcept { return config_.bandwidth_bps; }
    [[nodiscard]] double loss_probability() const noexcept { return config_.loss_probability; }
    [[nodiscard]] uint32_t queue_capacity() const noexcept { return config_.queue_capacity; }
    [[nodiscard]] ComponentState state() const noexcept { return config_.state; }

    void set_state(ComponentState state) noexcept { config_.state = state; }
    [[nodiscard]] bool is_available() const noexcept { return config_.state != ComponentState::DOWN; }

    // Calculates serialization delay given payload size in bytes
    [[nodiscard]] SimTime calculate_serialization_delay(size_t payload_bytes) const noexcept {
        if (config_.bandwidth_bps == 0 || payload_bytes == 0) {
            return 0;
        }
        // payload in bits * 1,000,000 us/sec / bandwidth_bps
        uint64_t bits = static_cast<uint64_t>(payload_bytes) * 8ULL;
        return static_cast<SimTime>((bits * 1'000'000ULL) / config_.bandwidth_bps);
    }

private:
    LinkConfig config_;
};

} // namespace ghostnet