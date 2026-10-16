#include <gtest/gtest.h>
#include "ghostnet/scenario_loader.hpp"

TEST(ScenarioLoaderTest, ParsesValidJsonConfiguration) {
    std::string json_data = R"({
        "config": {
            "seed": 999,
            "max_duration_us": 1000000,
            "max_events": 50000
        },
        "nodes": [
            {"id": 1, "name": "Client", "type": "CLIENT"},
            {"id": 2, "name": "Worker", "type": "SERVICE", "processing_delay_us": 2500}
        ],
        "links": [
            {"id": 10, "source_id": 1, "target_id": 2, "propagation_delay_us": 5000, "loss_probability": 0.0}
        ],
        "workloads": [
            {"source_id": 1, "target_id": 2, "arrival_rate_rps": 10.0, "duration_us": 1000000}
        ]
    })";

    auto scenario = ghostnet::ScenarioLoader::from_string(json_data);
    EXPECT_EQ(scenario.engine_config.seed, 999);
    EXPECT_EQ(scenario.topology.node_count(), 2);
    EXPECT_EQ(scenario.topology.link_count(), 1);
    EXPECT_EQ(scenario.workloads.size(), 1);

    ghostnet::SimulationEngine engine(scenario.engine_config, std::move(scenario.topology));
    for (const auto& w : scenario.workloads) {
        engine.add_workload(w);
    }
    auto status = engine.run();
    EXPECT_EQ(status, ghostnet::RunStatus::COMPLETED);

    auto m = engine.get_metrics();
    EXPECT_EQ(m.total_generated, 10);
    EXPECT_EQ(m.total_completed, 10);
    EXPECT_EQ(m.mean_latency_us, 7500.0); // 5000us link + 2500us service
}

TEST(ScenarioLoaderTest, RejectsInvalidTopologyJson) {
    // Missing target node 99
    std::string bad_json = R"({
        "nodes": [
            {"id": 1, "name": "Client", "type": "CLIENT"}
        ],
        "links": [
            {"id": 10, "source_id": 1, "target_id": 99}
        ]
    })";

    EXPECT_THROW(ghostnet::ScenarioLoader::from_string(bad_json), std::runtime_error);
}