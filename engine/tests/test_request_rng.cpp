#include <gtest/gtest.h>
#include "ghostnet/request.hpp"
#include "ghostnet/rng.hpp"

TEST(RequestTest, LifecycleTransitionsAndLatencyCalculation) {
    ghostnet::Request req({
        .id = 1,
        .source_id = 10,
        .target_id = 20,
        .creation_time = 1000,
        .timeout_duration = 50000,
        .payload_bytes = 128
    });

    EXPECT_EQ(req.state(), ghostnet::RequestState::GENERATED);
    EXPECT_FALSE(req.is_terminal());

    req.transition_to(ghostnet::RequestState::IN_TRANSIT, 1200);
    req.record_hop(10);
    EXPECT_EQ(req.state(), ghostnet::RequestState::IN_TRANSIT);

    req.transition_to(ghostnet::RequestState::COMPLETED, 2500);
    EXPECT_TRUE(req.is_terminal());
    EXPECT_TRUE(req.is_success());
    EXPECT_EQ(req.completion_time(), 2500);
    EXPECT_EQ(req.total_latency(), 1500); // 2500 - 1000

    // Transitioning from a terminal state must throw logic_error
    EXPECT_THROW(
        req.transition_to(ghostnet::RequestState::FAILED_TIMEOUT, 3000),
        std::logic_error
    );
}

TEST(RNGTest, SeedGuaranteesExactReproducibility) {
    const uint64_t seed = 987654321ULL;
    ghostnet::DeterministicRNG rng1(seed);
    ghostnet::DeterministicRNG rng2(seed);

    for (int i = 0; i < 1000; ++i) {
        EXPECT_DOUBLE_EQ(rng1.uniform_01(), rng2.uniform_01());
        EXPECT_EQ(rng1.evaluate_loss(0.05), rng2.evaluate_loss(0.05));
        EXPECT_EQ(rng1.uniform_int(10, 500), rng2.uniform_int(10, 500));
        EXPECT_EQ(rng1.exponential_interval_us(250.0), rng2.exponential_interval_us(250.0));
    }
}

TEST(RNGTest, LossProbabilityBoundaries) {
    ghostnet::DeterministicRNG rng(12345);

    for (int i = 0; i < 50; ++i) {
        EXPECT_FALSE(rng.evaluate_loss(0.0));
        EXPECT_TRUE(rng.evaluate_loss(1.0));
    }
}