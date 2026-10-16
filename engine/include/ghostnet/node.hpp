#pragma once

#include "ghostnet/event.hpp"
#include <string>
#include <cstdint>

namespace ghostnet {

enum class NodeType : uint8_t {
    CLIENT,
    SERVICE,
    ROUTER,
    LOAD_BALANCER,
    EXTERNAL_DEPENDENCY,
    NETWORK_GATEWAY
};

enum class ComponentState : uint8_t {
    UP,
    DEGRADED,
    DOWN
};

struct NodeConfig {
    ComponentId id{0};
    std::string name{};
    NodeType type{NodeType::SERVICE};
    
    // Processing & concurrency
    SimTime base_processing_delay{0}; // In microseconds
    uint32_t max_concurrency{1};      // Max requests in active processing simultaneously
    uint32_t queue_capacity{100};     // Maximum waiting requests before drop/rejection
    
    ComponentState state{ComponentState::UP};
};

class Node {
public:
    explicit Node(NodeConfig config) : config_(std::move(config)) {}

    [[nodiscard]] ComponentId id() const noexcept { return config_.id; }
    [[nodiscard]] const std::string& name() const noexcept { return config_.name; }
    [[nodiscard]] NodeType type() const noexcept { return config_.type; }
    [[nodiscard]] SimTime base_processing_delay() const noexcept { return config_.base_processing_delay; }
    [[nodiscard]] uint32_t max_concurrency() const noexcept { return config_.max_concurrency; }
    [[nodiscard]] uint32_t queue_capacity() const noexcept { return config_.queue_capacity; }
    [[nodiscard]] ComponentState state() const noexcept { return config_.state; }

    void set_state(ComponentState state) noexcept { config_.state = state; }
    [[nodiscard]] bool is_available() const noexcept { return config_.state != ComponentState::DOWN; }

private:
    NodeConfig config_;
};

} // namespace ghostnet