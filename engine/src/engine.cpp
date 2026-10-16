#include "ghostnet/engine.hpp"

namespace ghostnet {

SimulationEngine::SimulationEngine(EngineConfig config, Topology topology)
    : config_(config),
      topology_(std::move(topology)),
      scheduler_(),
      rng_(config.seed),
      metrics_(),
      status_(RunStatus::QUEUED),
      processed_events_(0),
      next_request_id_(1) {}

void SimulationEngine::add_workload(const WorkloadConfig& workload) {
    if (workload.arrival_rate_rps <= 0.0) return;

    SimTime t = 0;
    SimTime interval_us = static_cast<SimTime>(1'000'000.0 / workload.arrival_rate_rps);

    while (t < workload.duration_us) {
        RequestId req_id = next_request_id_++;
        requests_.emplace(req_id, Request({
            .id = req_id,
            .source_id = workload.source_id,
            .target_id = workload.target_id,
            .creation_time = t,
            .timeout_duration = workload.timeout_us,
            .payload_bytes = workload.payload_bytes
        }));

        scheduler_.schedule(t, EventType::REQUEST_GENERATED, workload.source_id, workload.target_id, req_id);

        if (workload.timeout_us > 0) {
            scheduler_.schedule(t + workload.timeout_us, EventType::TIMEOUT, workload.source_id, workload.target_id, req_id);
        }

        t += interval_us;
    }
}

RunStatus SimulationEngine::run() {
    status_ = RunStatus::RUNNING;

    while (auto opt_event = scheduler_.pop_next()) {
        const Event& event = *opt_event;

        if (event.timestamp > config_.max_duration_us) {
            status_ = RunStatus::COMPLETED;
            return status_;
        }

        if (processed_events_ >= config_.max_events) {
            status_ = RunStatus::LIMIT_REACHED;
            return status_;
        }

        handle_event(event);
        ++processed_events_;

        if (event_listener_) {
            event_listener_(event);
        }
    }

    status_ = RunStatus::COMPLETED;
    return status_;
}

void SimulationEngine::handle_event(const Event& event) {
    switch (event.type) {
        case EventType::REQUEST_GENERATED:
            handle_request_generated(event);
            break;
        case EventType::LINK_ARRIVAL:
            handle_link_arrival(event);
            break;
        case EventType::SERVICE_PROCESS_COMPLETE:
            handle_service_process_complete(event);
            break;
        case EventType::TIMEOUT:
            handle_timeout(event);
            break;
        default:
            break;
    }
}

void SimulationEngine::handle_request_generated(const Event& event) {
    auto req_it = requests_.find(event.request_id);
    if (req_it == requests_.end() || req_it->second.is_terminal()) {
        return;
    }
    Request& req = req_it->second;
    metrics_.record_generated();

    const Link* link = topology_.find_link(event.source_id, event.target_id);
    if (!link || !link->is_available()) {
        req.transition_to(RequestState::FAILED_ROUTING, scheduler_.current_time());
        metrics_.record_service_down();
        return;
    }

    // Evaluate packet loss
    if (rng_.evaluate_loss(link->loss_probability())) {
        req.transition_to(RequestState::FAILED_LOSS, scheduler_.current_time());
        metrics_.record_loss_drop();
        return;
    }

    // Schedule arrival across the link
    req.transition_to(RequestState::IN_TRANSIT, scheduler_.current_time());
    SimTime transit_time = link->propagation_delay() + link->calculate_serialization_delay(req.payload_bytes());
    SimTime arrival_time = scheduler_.current_time() + transit_time;

    scheduler_.schedule(arrival_time, EventType::LINK_ARRIVAL, event.source_id, event.target_id, req.id());
}

void SimulationEngine::handle_link_arrival(const Event& event) {
    auto req_it = requests_.find(event.request_id);
    if (req_it == requests_.end() || req_it->second.is_terminal()) {
        return;
    }
    Request& req = req_it->second;

    const Node* target_node = topology_.get_node(event.target_id);
    if (!target_node || !target_node->is_available()) {
        req.transition_to(RequestState::FAILED_SERVICE_DOWN, scheduler_.current_time());
        metrics_.record_service_down();
        return;
    }

    req.transition_to(RequestState::PROCESSING, scheduler_.current_time());
    SimTime finish_time = scheduler_.current_time() + target_node->base_processing_delay();

    scheduler_.schedule(finish_time, EventType::SERVICE_PROCESS_COMPLETE, event.source_id, event.target_id, req.id());
}

void SimulationEngine::handle_service_process_complete(const Event& event) {
    auto req_it = requests_.find(event.request_id);
    if (req_it == requests_.end() || req_it->second.is_terminal()) {
        return;
    }
    Request& req = req_it->second;

    req.transition_to(RequestState::COMPLETED, scheduler_.current_time());
    metrics_.record_completion(req.total_latency());
}

void SimulationEngine::handle_timeout(const Event& event) {
    auto req_it = requests_.find(event.request_id);
    if (req_it == requests_.end() || req_it->second.is_terminal()) {
        return;
    }
    Request& req = req_it->second;

    req.transition_to(RequestState::FAILED_TIMEOUT, scheduler_.current_time());
    metrics_.record_timeout();
}

} // namespace ghostnet