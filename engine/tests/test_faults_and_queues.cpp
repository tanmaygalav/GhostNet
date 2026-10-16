#include <gtest/gtest.h>
#include "ghostnet/engine.hpp"

ghostnet::Topology make_topo_with_limits(uint32_t concurrency, uint32_t queue_cap, ghostnet::SimTime proc_delay) {
    ghostnet::Topology topo;
    topo.add_node({.id = 1, .name = "Client", .type = ghostnet::NodeType::CLIENT});
    topo.add_node({
        .id = 2,
        .name = "Worker",
        .type = ghostnet::NodeType::SERVICE,
        .base_processing_delay = proc_delay,
        .max_concurrency = concurrency,
        .queue_capacity = queue_cap
    });
    topo.add_link({
        .id = 10,
        .source_id = 1,
        .target_id = 2,
        .propagation_delay = 1000 // 1ms
    });
    return topo;
}

TEST(FaultAndQueueTest, QueueOverflowTriggersOnOverload) {
    auto topo = make_topo_with_limits(1, 1, 100'000);
    ghostnet::EngineConfig cfg{.seed = 42, .max_duration_us = 1'000'000};
    ghostnet::SimulationEngine engine(cfg, std::move(topo));

    engine.add_workload({
        .source_id = 1,
        .target_id = 2,
        .arrival_rate_rps = 100.0,
        .duration_us = 100'000,
        .timeout_us = 500'000
    });

    engine.run();
    auto m = engine.get_metrics();

    EXPECT_GT(m.queue_overflows, 0);
    EXPECT_EQ(m.total_generated, 10);
    EXPECT_GT(m.total_failed, 0);
}

TEST(FaultAndQueueTest, ServiceOutageFailsRequestsDuringDowntimeAndRecovers) {
    auto topo = make_topo_with_limits(5, 50, 10'000); // 10ms processing
    ghostnet::EngineConfig cfg{.seed = 42, .max_duration_us = 2'000'000};
    ghostnet::SimulationEngine engine(cfg, std::move(topo));

    engine.add_workload({
        .source_id = 1,
        .target_id = 2,
        .arrival_rate_rps = 10.0,
        .duration_us = 2'000'000,
        .timeout_us = 500'000
    });

    engine.schedule_fault({
        .id = 1,
        .type = ghostnet::FaultType::SERVICE_OUTAGE,
        .target_component_id = 2,
        .start_time_us = 450'000,
        .duration_us = 600'000
    });

    engine.run();
    auto m = engine.get_metrics();

    EXPECT_GT(m.service_down_failures, 0);
    EXPECT_GT(m.total_completed, 0);
    EXPECT_EQ(m.total_completed + m.total_failed, m.total_generated);
}

TEST(FaultAndQueueTest, LatencySpikeImpairsLinkDelay) {
    auto topo = make_topo_with_limits(5, 50, 2'000); // base proc delay 2ms
    ghostnet::EngineConfig cfg{.seed = 42, .max_duration_us = 1'000'000};
    ghostnet::SimulationEngine engine(cfg, std::move(topo));

    engine.add_workload({
        .source_id = 1,
        .target_id = 2,
        .arrival_rate_rps = 5.0,
        .duration_us = 500'000,
        .timeout_us = 1'000'000
    });

    engine.schedule_fault({
        .id = 100,
        .type = ghostnet::FaultType::LATENCY_SPIKE,
        .target_component_id = 10,
        .start_time_us = 150'000,
        .duration_us = 200'000,
        .extra_latency_us = 50'000
    });

    engine.run();
    auto m = engine.get_metrics();

    EXPECT_EQ(m.total_completed, 3);
    EXPECT_EQ(m.min_latency_us, 3000);
    EXPECT_EQ(m.max_latency_us, 53000);
}