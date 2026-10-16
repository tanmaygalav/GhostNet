#include "ghostnet/scenario_loader.hpp"
#include <stdexcept>

namespace ghostnet {

static NodeType parse_node_type(const std::string& type_str) {
    if (type_str == "CLIENT") return NodeType::CLIENT;
    if (type_str == "SERVICE") return NodeType::SERVICE;
    if (type_str == "ROUTER") return NodeType::ROUTER;
    if (type_str == "LOAD_BALANCER") return NodeType::LOAD_BALANCER;
    if (type_str == "EXTERNAL_DEPENDENCY") return NodeType::EXTERNAL_DEPENDENCY;
    if (type_str == "NETWORK_GATEWAY") return NodeType::NETWORK_GATEWAY;
    return NodeType::SERVICE;
}

static FaultType parse_fault_type(const std::string& type_str) {
    if (type_str == "LATENCY_SPIKE") return FaultType::LATENCY_SPIKE;
    return FaultType::SERVICE_OUTAGE;
}

ParsedScenario ScenarioLoader::from_json(const nlohmann::json& j) {
    ParsedScenario scenario;

    // 1. Engine configuration
    if (j.contains("config")) {
        const auto& cfg = j["config"];
        scenario.engine_config.seed = cfg.value("seed", 42ULL);
        scenario.engine_config.max_duration_us = cfg.value("max_duration_us", 10'000'000ULL);
        scenario.engine_config.max_events = cfg.value("max_events", 1'000'000ULL);
    }

    // 2. Topology nodes
    if (j.contains("nodes") && j["nodes"].is_array()) {
        for (const auto& item : j["nodes"]) {
            NodeConfig node;
            node.id = item.at("id").get<ComponentId>();
            node.name = item.value("name", "Node-" + std::to_string(node.id));
            node.type = parse_node_type(item.value("type", "SERVICE"));
            node.base_processing_delay = item.value("processing_delay_us", 0ULL);
            node.max_concurrency = item.value("max_concurrency", 1U);
            node.queue_capacity = item.value("queue_capacity", 100U);
            
            if (!scenario.topology.add_node(std::move(node))) {
                throw std::runtime_error("Duplicate node ID: " + std::to_string(node.id));
            }
        }
    }

    // 3. Topology links
    if (j.contains("links") && j["links"].is_array()) {
        for (const auto& item : j["links"]) {
            LinkConfig link;
            link.id = item.at("id").get<LinkId>();
            link.source_id = item.at("source_id").get<ComponentId>();
            link.target_id = item.at("target_id").get<ComponentId>();
            link.propagation_delay = item.value("propagation_delay_us", 0ULL);
            link.bandwidth_bps = item.value("bandwidth_bps", 0ULL);
            link.loss_probability = item.value("loss_probability", 0.0);
            link.queue_capacity = item.value("queue_capacity", 1000U);

            if (!scenario.topology.add_link(std::move(link))) {
                throw std::runtime_error("Duplicate link ID: " + std::to_string(link.id));
            }
        }
    }

    auto validation = scenario.topology.validate();
    if (!validation.valid) {
        std::string err_msg = "Invalid topology: ";
        for (const auto& err : validation.errors) {
            err_msg += "[" + err + "] ";
        }
        throw std::runtime_error(err_msg);
    }

    // 4. Workloads
    if (j.contains("workloads") && j["workloads"].is_array()) {
        for (const auto& item : j["workloads"]) {
            WorkloadConfig w;
            w.source_id = item.at("source_id").get<ComponentId>();
            w.target_id = item.at("target_id").get<ComponentId>();
            w.arrival_rate_rps = item.value("arrival_rate_rps", 10.0);
            w.duration_us = item.value("duration_us", scenario.engine_config.max_duration_us);
            w.timeout_us = item.value("timeout_us", 1'000'000ULL);
            w.payload_bytes = item.value("payload_bytes", 64ULL);
            scenario.workloads.push_back(w);
        }
    }

    // 5. Fault Schedules
    if (j.contains("faults") && j["faults"].is_array()) {
        for (const auto& item : j["faults"]) {
            FaultConfig fault;
            fault.id = item.at("id").get<uint32_t>();
            fault.type = parse_fault_type(item.value("type", "SERVICE_OUTAGE"));
            fault.target_component_id = item.at("target_component_id").get<ComponentId>();
            fault.start_time_us = item.at("start_time_us").get<SimTime>();
            fault.duration_us = item.at("duration_us").get<SimTime>();
            fault.extra_latency_us = item.value("extra_latency_us", 0ULL);
            scenario.faults.push_back(fault);
        }
    }

    return scenario;
}

ParsedScenario ScenarioLoader::from_string(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    return from_json(j);
}

nlohmann::json ScenarioLoader::metrics_to_json(
    RunStatus status, SimTime duration_us, uint64_t processed_events, const MetricsSummary& summary) 
{
    std::string status_str = "COMPLETED";
    if (status == RunStatus::LIMIT_REACHED) status_str = "LIMIT_REACHED";
    else if (status == RunStatus::FAILED) status_str = "FAILED";

    return nlohmann::json{
        {"status", status_str},
        {"virtual_duration_us", duration_us},
        {"processed_events", processed_events},
        {"metrics", {
            {"total_generated", summary.total_generated},
            {"total_completed", summary.total_completed},
            {"total_failed", summary.total_failed},
            {"dropped_by_loss", summary.dropped_by_loss},
            {"timeouts", summary.timeouts},
            {"queue_overflows", summary.queue_overflows},
            {"service_down_failures", summary.service_down_failures},
            {"completion_rate", summary.completion_rate},
            {"mean_latency_us", summary.mean_latency_us},
            {"min_latency_us", summary.min_latency_us},
            {"p50_latency_us", summary.p50_latency_us},
            {"p95_latency_us", summary.p95_latency_us},
            {"p99_latency_us", summary.p99_latency_us},
            {"max_latency_us", summary.max_latency_us}
        }}
    };
}

} // namespace ghostnet