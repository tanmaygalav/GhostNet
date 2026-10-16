#pragma once

#include "ghostnet/engine.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>

namespace ghostnet {

struct ParsedScenario {
    EngineConfig engine_config;
    Topology topology;
    std::vector<WorkloadConfig> workloads;
};

class ScenarioLoader {
public:
    // Parses JSON string into engine parameters, topology, and workloads
    static ParsedScenario from_json(const nlohmann::json& j);

    // Parses JSON directly from a string buffer
    static ParsedScenario from_string(const std::string& json_str);

    // Converts execution metrics and run status to a JSON object
    static nlohmann::json metrics_to_json(RunStatus status, SimTime duration_us, uint64_t processed_events, const MetricsSummary& summary);
};

} // namespace ghostnet