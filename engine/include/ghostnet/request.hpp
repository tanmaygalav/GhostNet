#pragma once

#include "ghostnet/event.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

namespace ghostnet {

enum class RequestState : uint8_t {
    GENERATED,
    LINK_QUEUE,
    IN_TRANSIT,
    SERVICE_QUEUE,
    PROCESSING,
    COMPLETED,
    FAILED_TIMEOUT,
    FAILED_LOSS,
    FAILED_SERVICE_DOWN,
    FAILED_QUEUE_OVERFLOW,
    FAILED_ROUTING,
    CANCELLED
};

inline bool is_terminal_state(RequestState state) noexcept {
    switch (state) {
        case RequestState::COMPLETED:
        case RequestState::FAILED_TIMEOUT:
        case RequestState::FAILED_LOSS:
        case RequestState::FAILED_SERVICE_DOWN:
        case RequestState::FAILED_QUEUE_OVERFLOW:
        case RequestState::FAILED_ROUTING:
        case RequestState::CANCELLED:
            return true;
        default:
            return false;
    }
}

inline const char* request_state_to_string(RequestState state) noexcept {
    switch (state) {
        case RequestState::GENERATED: return "GENERATED";
        case RequestState::LINK_QUEUE: return "LINK_QUEUE";
        case RequestState::IN_TRANSIT: return "IN_TRANSIT";
        case RequestState::SERVICE_QUEUE: return "SERVICE_QUEUE";
        case RequestState::PROCESSING: return "PROCESSING";
        case RequestState::COMPLETED: return "COMPLETED";
        case RequestState::FAILED_TIMEOUT: return "FAILED_TIMEOUT";
        case RequestState::FAILED_LOSS: return "FAILED_LOSS";
        case RequestState::FAILED_SERVICE_DOWN: return "FAILED_SERVICE_DOWN";
        case RequestState::FAILED_QUEUE_OVERFLOW: return "FAILED_QUEUE_OVERFLOW";
        case RequestState::FAILED_ROUTING: return "FAILED_ROUTING";
        case RequestState::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

struct RequestConfig {
    RequestId id{0};
    ComponentId source_id{0};
    ComponentId target_id{0};
    SimTime creation_time{0};
    SimTime timeout_duration{0}; // 0 = no timeout
    size_t payload_bytes{64};
};

class Request {
public:
    explicit Request(RequestConfig config)
        : config_(config), state_(RequestState::GENERATED), attempt_count_(1) {}

    [[nodiscard]] RequestId id() const noexcept { return config_.id; }
    [[nodiscard]] ComponentId source_id() const noexcept { return config_.source_id; }
    [[nodiscard]] ComponentId target_id() const noexcept { return config_.target_id; }
    [[nodiscard]] SimTime creation_time() const noexcept { return config_.creation_time; }
    [[nodiscard]] SimTime timeout_duration() const noexcept { return config_.timeout_duration; }
    [[nodiscard]] size_t payload_bytes() const noexcept { return config_.payload_bytes; }

    [[nodiscard]] RequestState state() const noexcept { return state_; }
    [[nodiscard]] uint32_t attempt_count() const noexcept { return attempt_count_; }
    [[nodiscard]] SimTime completion_time() const noexcept { return completion_time_; }

    [[nodiscard]] bool is_terminal() const noexcept { return is_terminal_state(state_); }
    [[nodiscard]] bool is_success() const noexcept { return state_ == RequestState::COMPLETED; }

    [[nodiscard]] SimTime total_latency() const noexcept {
        if (!is_terminal() || completion_time_ < config_.creation_time) {
            return 0;
        }
        return completion_time_ - config_.creation_time;
    }

    // State transition guard
    void transition_to(RequestState new_state, SimTime current_time) {
        if (is_terminal()) {
            throw std::logic_error("Cannot transition request from a terminal state.");
        }

        state_ = new_state;
        if (is_terminal_state(new_state)) {
            completion_time_ = current_time;
        }
    }

    void record_attempt() noexcept {
        ++attempt_count_;
    }

    void record_hop(ComponentId component_id) {
        hop_history_.push_back(component_id);
    }

    [[nodiscard]] const std::vector<ComponentId>& hop_history() const noexcept {
        return hop_history_;
    }

private:
    RequestConfig config_;
    RequestState state_;
    uint32_t attempt_count_{1};
    SimTime completion_time_{0};
    std::vector<ComponentId> hop_history_{};
};

} // namespace ghostnet