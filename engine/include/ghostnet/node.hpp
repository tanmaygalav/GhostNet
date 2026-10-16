#pragma once

#include "ghostnet/event.hpp"
#include <string>
#include <cstdint>
#include <queue>
#include <optional>

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
    
    SimTime base_processing_delay{0}; // in microseconds
    uint32_t max_concurrency{1};      // parallel execution capacity
    uint32_t queue_capacity{100};     // bounded queue limit
    
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

    // Concurrency & Queue Management
    [[nodiscard]] uint32_t active_workers() const noexcept { return active_workers_; }
    [[nodiscard]] size_t current_queue_depth() const noexcept { return request_queue_.size(); }
    [[nodiscard]] bool has_free_worker() const noexcept { return active_workers_ < config_.max_concurrency; }
    [[nodiscard]] bool is_queue_full() const noexcept { return request_queue_.size() >= config_.queue_capacity; }

    void acquire_worker() noexcept {
        if (active_workers_ < config_.max_concurrency) {
            ++active_workers_;
        }
    }

    void release_worker() noexcept {
        if (active_workers_ > 0) {
            --active_workers_;
        }
    }

    bool enqueue_request(RequestId req_id) {
        if (is_queue_full()) {
            return false;
        }
        request_queue_.push(req_id);
        return true;
    }

    std::optional<RequestId> dequeue_request() {
        if (request_queue_.empty()) {
            return std::nullopt;
        }
        RequestId req_id = request_queue_.front();
        request_queue_.pop();
        return req_id;
    }

    void clear_queue() {
        while (!request_queue_.empty()) {
            request_queue_.pop();
        }
        active_workers_ = 0;
    }

private:
    NodeConfig config_;
    uint32_t active_workers_{0};
    std::queue<RequestId> request_queue_{};
};

} // namespace ghostnet