#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <compare>

namespace ghostnet {

using SimTime = uint64_t;      // Virtual time in microseconds
using EventId = uint64_t;
using ComponentId = uint32_t;
using RequestId = uint64_t;

enum class EventType : uint8_t {
    REQUEST_GENERATED,
    REQUEST_TRANSMITTED,
    LINK_ARRIVAL,
    QUEUE_ENTRY,
    SERVICE_PROCESS_START,
    SERVICE_PROCESS_COMPLETE,
    TIMEOUT,
    REQUEST_FAILED,
    RETRY_SCHEDULED,
    FAULT_ACTIVATED,
    FAULT_RECOVERED,
    REQUEST_COMPLETED
};

struct Event {
    SimTime timestamp{0};
    uint64_t sequence_number{0};
    EventType type{EventType::REQUEST_GENERATED};
    ComponentId source_id{0};
    ComponentId target_id{0};
    RequestId request_id{0};
    uint32_t fault_id{0};
    std::string payload{};

    // Deterministic ordering: earliest timestamp first, ties broken by sequence number
    auto operator<=>(const Event& other) const {
        if (auto cmp = timestamp <=> other.timestamp; cmp != 0) {
            return cmp;
        }
        return sequence_number <=> other.sequence_number;
    }

    bool operator==(const Event& other) const = default;
};

// Comparator for std::priority_queue (which orders largest first by default)
// We need the smallest (earliest) event on top.
struct EventComparator {
    bool operator()(const Event& a, const Event& b) const {
        return a > b;
    }
};

} // namespace ghostnet