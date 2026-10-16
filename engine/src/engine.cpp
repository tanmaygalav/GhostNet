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

void SimulationEngine::schedule_fault(const FaultConfig& fault) {
    registered_faults_[fault.id] = fault;
    
    // Schedule activation
    scheduler_.schedule(fault.start_time_us, EventType::FAULT_ACTIVATED, 0, fault.target_component_id, 0, fault.id);
    
    // Schedule recovery
    SimTime recovery_time = fault.start_time_us + fault.duration_us;
    scheduler_.schedule(recovery_time, EventType::FAULT_RECOVERED, 0, fault.target_component_id, 0, fault.id);
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
        case EventType::FAULT_ACTIVATED:
            handle_fault_activated(event);
            break;
        case EventType::FAULT_RECOVERED:
            handle_fault_recovered(event);
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

    if (rng_.evaluate_loss(link->loss_probability())) {
        req.transition_to(RequestState::FAILED_LOSS, scheduler_.current_time());
        metrics_.record_loss_drop();
        return;
    }

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

    Node* target_node = topology_.get_node_mut(event.target_id);
    if (!target_node || !target_node->is_available()) {
        req.transition_to(RequestState::FAILED_SERVICE_DOWN, scheduler_.current_time());
        metrics_.record_service_down();
        return;
    }

    // Try acquiring worker immediately
    if (target_node->has_free_worker()) {
        target_node->acquire_worker();
        req.transition_to(RequestState::PROCESSING, scheduler_.current_time());
        SimTime finish_time = scheduler_.current_time() + target_node->base_processing_delay();
        scheduler_.schedule(finish_time, EventType::SERVICE_PROCESS_COMPLETE, event.source_id, event.target_id, req.id());
    } else {
        // Enqueue request
        if (!target_node->enqueue_request(req.id())) {
            // Queue full -> Backpressure failure
            req.transition_to(RequestState::FAILED_QUEUE_OVERFLOW, scheduler_.current_time());
            metrics_.record_queue_overflow();
            return;
        }
        req.transition_to(RequestState::SERVICE_QUEUE, scheduler_.current_time());
    }
}

void SimulationEngine::handle_service_process_complete(const Event& event) {
    Node* target_node = topology_.get_node_mut(event.target_id);
    if (target_node) {
        target_node->release_worker();
    }

    auto req_it = requests_.find(event.request_id);
    if (req_it != requests_.end() && !req_it->second.is_terminal()) {
        Request& req = req_it->second;
        req.transition_to(RequestState::COMPLETED, scheduler_.current_time());
        metrics_.record_completion(req.total_latency());
    }

    // If there are waiting requests in queue, dispatch next one
    if (target_node) {
        dispatch_service_work(target_node);
    }
}

void SimulationEngine::dispatch_service_work(Node* service_node) {
    while (service_node->has_free_worker() && service_node->current_queue_depth() > 0) {
        auto next_req_opt = service_node->dequeue_request();
        if (!next_req_opt) break;

        RequestId next_id = *next_req_opt;
        auto req_it = requests_.find(next_id);
        if (req_it == requests_.end() || req_it->second.is_terminal()) {
            continue; // Skip timed-out/cancelled requests that sat in queue
        }

        service_node->acquire_worker();
        Request& waiting_req = req_it->second;
        waiting_req.transition_to(RequestState::PROCESSING, scheduler_.current_time());

        SimTime finish_time = scheduler_.current_time() + service_node->base_processing_delay();
        scheduler_.schedule(finish_time, EventType::SERVICE_PROCESS_COMPLETE, waiting_req.source_id(), service_node->id(), next_id);
    }
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

void SimulationEngine::handle_fault_activated(const Event& event) {
    auto it = registered_faults_.find(event.fault_id);
    if (it == registered_faults_.end()) return;

    const FaultConfig& fault = it->second;
    if (fault.type == FaultType::SERVICE_OUTAGE) {
        Node* node = topology_.get_node_mut(fault.target_component_id);
        if (node) {
            node->set_state(ComponentState::DOWN);
            // Drop queued items during catastrophic outage
            node->clear_queue();
        }
    } else if (fault.type == FaultType::LATENCY_SPIKE) {
        // Target component is Link ID
        auto* link = const_cast<Link*>(topology_.get_link(fault.target_component_id));
        if (link) {
            link->set_extra_latency(fault.extra_latency_us);
        }
    }
}

void SimulationEngine::handle_fault_recovered(const Event& event) {
    auto it = registered_faults_.find(event.fault_id);
    if (it == registered_faults_.end()) return;

    const FaultConfig& fault = it->second;
    if (fault.type == FaultType::SERVICE_OUTAGE) {
        Node* node = topology_.get_node_mut(fault.target_component_id);
        if (node) {
            node->set_state(ComponentState::UP);
        }
    } else if (fault.type == FaultType::LATENCY_SPIKE) {
        auto* link = const_cast<Link*>(topology_.get_link(fault.target_component_id));
        if (link) {
            link->clear_extra_latency();
        }
    }
}

} // namespace ghostnet