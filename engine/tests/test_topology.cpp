#include <gtest/gtest.h>
#include "ghostnet/topology.hpp"

TEST(TopologyTest, NodeAndLinkManagement) {
    ghostnet::Topology topo;

    EXPECT_TRUE(topo.add_node({.id = 1, .name = "Client-1", .type = ghostnet::NodeType::CLIENT}));
    EXPECT_TRUE(topo.add_node({.id = 2, .name = "Service-Auth", .type = ghostnet::NodeType::SERVICE, .base_processing_delay = 5000}));
    
    // Duplicate node ID rejection
    EXPECT_FALSE(topo.add_node({.id = 1, .name = "Duplicate"}));

    EXPECT_TRUE(topo.add_link({.id = 101, .source_id = 1, .target_id = 2, .propagation_delay = 12000}));
    EXPECT_FALSE(topo.add_link({.id = 101, .source_id = 1, .target_id = 2})); // Duplicate link ID

    EXPECT_EQ(topo.node_count(), 2);
    EXPECT_EQ(topo.link_count(), 1);

    const auto* found_link = topo.find_link(1, 2);
    ASSERT_NE(found_link, nullptr);
    EXPECT_EQ(found_link->propagation_delay(), 12000);
}

TEST(TopologyTest, SerializationDelayCalculation) {
    ghostnet::Link link({
        .id = 1,
        .source_id = 1,
        .target_id = 2,
        .propagation_delay = 1000,
        .bandwidth_bps = 8'000'000 // 8 Mbps = 1 MB/s
    });

    // 1000 bytes @ 8 Mbps = 8,000 bits / 8,000,000 bits/sec = 0.001 sec = 1000 microseconds
    EXPECT_EQ(link.calculate_serialization_delay(1000), 1000);
}

TEST(TopologyTest, ValidationCatchesOrphanedLinksAndSelfLoops) {
    ghostnet::Topology topo;
    topo.add_node({.id = 1, .name = "Node-1"});

    // Link references non-existent node 99
    topo.add_link({.id = 1, .source_id = 1, .target_id = 99});
    auto res = topo.validate();
    EXPECT_FALSE(res.valid);
    EXPECT_FALSE(res.errors.empty());

    topo.clear();
    topo.add_node({.id = 1, .name = "Node-1"});
    // Self-loop
    topo.add_link({.id = 2, .source_id = 1, .target_id = 1});
    res = topo.validate();
    EXPECT_FALSE(res.valid);
}