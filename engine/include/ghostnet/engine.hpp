#pragma once

#include "ghostnet/scheduler.hpp"
#include "ghostnet/topology.hpp"
#include "ghostnet/request.hpp"
#include "ghostnet/rng.hpp"
#include "ghostnet/metrics.hpp"
#include "ghostnet/fault.hpp"
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

namespace ghostnet {

enum class RunStatus : uint8_t {
    QUEUED,
    RUNNING,
    COMPLETED,
    LIMIT_REACHED,
    FAILED
};

struct EngineConfig {
    uint64_t seed{42};
    SimTime max_duration_us{10'000'000};
    uint64_t max_events{1'000'000};
};

struct WorkloadConfig {
    ComponentId source_id{0};
    ComponentId target_id{0};
    double arrival_rate_rps{10.0};
    SimTime duration_us{10'000'000};
    SimTime timeout_us{1'000'000};
    size_t payload_bytes{64};
};

using EventCallback = std::function<void(const Event&)>;

class SimulationEngine {
public:
    explicit SimulationEngine(EngineConfig config, Topology topology);

    void set_event_listener(EventCallback listener) { event_listener_ = std::move(listener); }

    void add_workload(const WorkloadConfig& workload);
    void schedule_fault(const FaultConfig& fault);

    RunStatus run();

    [[nodiscard]] MetricsSummary get_metrics() const { return metrics_.summarize(); }
    [[nodiscard]] SimTime current_time() const noexcept { return scheduler_.current_time(); }
    [[nodiscard]] uint64_t processed_event_count() const noexcept { return processed_events_; }
    [[nodiscard]] RunStatus status() const noexcept { return status_; }

private:
    void handle_event(const Event& event);
    void handle_request_generated(const Event& event);
    void handle_link_arrival(const Event& event);
    void handle_service_process_complete(const Event& event);
    void handle_timeout(const Event& event);
    void handle_fault_activated(const Event& event);
    void handle_fault_recovered(const Event& event);

    void dispatch_service_work(Node* service_node);

    EngineConfig config_;
    Topology topology_;
    Scheduler scheduler_;
    DeterministicRNG rng_;
    MetricsAccumulator metrics_;

    RunStatus status_{RunStatus::QUEUED};
    uint64_t processed_events_{0};
    RequestId next_request_id_{1};

    std::unordered_map<RequestId, Request> requests_;
    std::unordered_map<uint32_t, FaultConfig> registered_faults_;
    EventCallback event_listener_{nullptr};
};

} // namespace ghostnet