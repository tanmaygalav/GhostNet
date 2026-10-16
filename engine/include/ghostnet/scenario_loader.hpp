#pragma once

#include "ghostnet/engine.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ghostnet {

struct ParsedScenario {
    EngineConfig engine_config;
    Topology topology;
    std::vector<WorkloadConfig> workloads;
    std::vector<FaultConfig> faults;
};

class ScenarioLoader {
public:
    static ParsedScenario from_json(const nlohmann::json& j);
    static ParsedScenario from_string(const std::string& json_str);
    static nlohmann::json metrics_to_json(RunStatus status, SimTime duration_us, uint64_t processed_events, const MetricsSummary& summary);
};

} // namespace ghostnet