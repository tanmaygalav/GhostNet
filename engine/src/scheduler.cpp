#include "ghostnet/scheduler.hpp"

namespace ghostnet {

void Scheduler::reset() {
    current_time_ = 0;
    next_sequence_ = 0;
    while (!queue_.empty()) {
        queue_.pop();
    }
}

void Scheduler::schedule(SimTime timestamp, EventType type, ComponentId source, ComponentId target,
                         RequestId req_id, uint32_t fault_id, std::string payload) {
    if (timestamp < current_time_) {
        throw std::invalid_argument("Cannot schedule event in the past: timestamp < current_time");
    }

    Event event{
        .timestamp = timestamp,
        .sequence_number = ++next_sequence_,
        .type = type,
        .source_id = source,
        .target_id = target,
        .request_id = req_id,
        .fault_id = fault_id,
        .payload = std::move(payload)
    };

    queue_.push(std::move(event));
}

void Scheduler::schedule(Event event) {
    if (event.timestamp < current_time_) {
        throw std::invalid_argument("Cannot schedule event in the past: timestamp < current_time");
    }
    event.sequence_number = ++next_sequence_;
    queue_.push(std::move(event));
}

std::optional<Event> Scheduler::pop_next() {
    if (queue_.empty()) {
        return std::nullopt;
    }

    Event top_event = queue_.top();
    queue_.pop();

    // Advance virtual clock strictly monotonically
    current_time_ = top_event.timestamp;
    return top_event;
}

std::optional<Event> Scheduler::peek_next() const {
    if (queue_.empty()) {
        return std::nullopt;
    }
    return queue_.top();
}

} // namespace ghostnet