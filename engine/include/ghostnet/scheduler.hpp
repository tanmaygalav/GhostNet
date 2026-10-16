#pragma once

#include "ghostnet/event.hpp"
#include <queue>
#include <optional>
#include <stdexcept>
#include <vector>

namespace ghostnet {

class Scheduler {
public:
    Scheduler() = default;

    // Resets virtual clock, queue, and sequence counter
    void reset();

    // Schedules a new event; rejects events set in the past relative to current_time
    void schedule(SimTime timestamp, EventType type, ComponentId source, ComponentId target,
                  RequestId req_id = 0, uint32_t fault_id = 0, std::string payload = "");

    // Directly schedule an existing event struct with validation
    void schedule(Event event);

    // Pops the next earliest event and advances current virtual time
    std::optional<Event> pop_next();

    // Peek next event without advancing state
    [[nodiscard]] std::optional<Event> peek_next() const;

    [[nodiscard]] SimTime current_time() const noexcept { return current_time_; }
    [[nodiscard]] size_t pending_event_count() const noexcept { return queue_.size(); }
    [[nodiscard]] bool empty() const noexcept { return queue_.empty(); }

private:
    SimTime current_time_{0};
    uint64_t next_sequence_{0};
    std::priority_queue<Event, std::vector<Event>, EventComparator> queue_;
};

} // namespace ghostnet