#include <gtest/gtest.h>
#include "ghostnet/engine.hpp"

// Setup a simple 2-node topology: Client(1) -> Link(101) -> Service(2)
ghostnet::Topology create_two_node_topology(SimTime delay_us, double loss_prob, SimTime proc_delay_us) {
    ghostnet::Topology topo;
    topo.add_node({.id = 1, .name = "Client", .type = ghostnet::NodeType::CLIENT});
    topo.add_node({.id = 2, .name = "Backend", .type = ghostnet::NodeType::SERVICE, .base_processing_delay = proc_delay_us});
    topo.add_link({
        .id = 101,
        .source_id = 1,
        .target_id = 2,
        .propagation_delay = delay_us,
        .bandwidth_bps = 10'000'000,
        .loss_probability = loss_prob
    });
    return topo;
}

TEST(EngineTest, SimulatesRequestsWithCorrectLatency) {
    auto topo = create_two_node_topology(10'000, 0.0, 5'000); // 10ms link delay + 5ms proc delay
    ghostnet::EngineConfig cfg{.seed = 42, .max_duration_us = 2'000'000};
    ghostnet::SimulationEngine engine(cfg, std::move(topo));

    // Send 5 requests at 2 req/sec (1 every 500,000 us)
    engine.add_workload({
        .source_id = 1,
        .target_id = 2,
        .arrival_rate_rps = 2.0,
        .duration_us = 2'000'000,
        .timeout_us = 500'000,
        .payload_bytes = 0
    });

    auto status = engine.run();
    EXPECT_EQ(status, ghostnet::RunStatus::COMPLETED);

    auto metrics = engine.get_metrics();
    EXPECT_EQ(metrics.total_generated, 4); // 0, 500ms, 1000ms, 1500ms
    EXPECT_EQ(metrics.total_completed, 4);
    EXPECT_EQ(metrics.total_failed, 0);

    // Latency = 10ms + 5ms = 15,000 us
    EXPECT_EQ(metrics.mean_latency_us, 15'000.0);
    EXPECT_EQ(metrics.p50_latency_us, 15'000);
    EXPECT_EQ(metrics.p99_latency_us, 15'000);
}

TEST(EngineTest, DropsRequestsUnderPacketLoss) {
    auto topo = create_two_node_topology(1'000, 1.0, 1'000); // 100% loss
    ghostnet::EngineConfig cfg{.seed = 1234, .max_duration_us = 1'000'000};
    ghostnet::SimulationEngine engine(cfg, std::move(topo));

    engine.add_workload({
        .source_id = 1,
        .target_id = 2,
        .arrival_rate_rps = 10.0,
        .duration_us = 1'000'000,
        .timeout_us = 500'000
    });

    engine.run();
    auto metrics = engine.get_metrics();
    EXPECT_EQ(metrics.total_generated, 10);
    EXPECT_EQ(metrics.total_completed, 0);
    EXPECT_EQ(metrics.dropped_by_loss, 10);
    EXPECT_DOUBLE_EQ(metrics.completion_rate, 0.0);
}

TEST(EngineTest, StrictDeterminismAcrossIdenticalRuns) {
    auto run_sim = [](uint64_t seed) {
        auto topo = create_two_node_topology(5'000, 0.25, 3'000); // 25% loss
        ghostnet::EngineConfig cfg{.seed = seed, .max_duration_us = 5'000'000};
        ghostnet::SimulationEngine engine(cfg, std::move(topo));

        engine.add_workload({
            .source_id = 1,
            .target_id = 2,
            .arrival_rate_rps = 20.0,
            .duration_us = 5'000'000,
            .timeout_us = 100'000
        });

        engine.run();
        return engine.get_metrics();
    };

    auto m1 = run_sim(777);
    auto m2 = run_sim(777);

    // Guaranteed bit-for-bit equivalence
    EXPECT_EQ(m1.total_generated, m2.total_generated);
    EXPECT_EQ(m1.total_completed, m2.total_completed);
    EXPECT_EQ(m1.total_failed, m2.total_failed);
    EXPECT_EQ(m1.dropped_by_loss, m2.dropped_by_loss);
    EXPECT_DOUBLE_EQ(m1.mean_latency_us, m2.mean_latency_us);
    EXPECT_EQ(m1.p95_latency_us, m2.p95_latency_us);
}