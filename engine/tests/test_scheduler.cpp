#include <gtest/gtest.h>
#include "ghostnet/scheduler.hpp"

TEST(SchedulerTest, OrdersEventsChronologically) {
    ghostnet::Scheduler scheduler;

    scheduler.schedule(300, ghostnet::EventType::REQUEST_COMPLETED, 1, 2, 100);
    scheduler.schedule(100, ghostnet::EventType::REQUEST_GENERATED, 1, 2, 101);
    scheduler.schedule(200, ghostnet::EventType::REQUEST_TRANSMITTED, 1, 2, 102);

    auto e1 = scheduler.pop_next();
    ASSERT_TRUE(e1.has_value());
    EXPECT_EQ(e1->timestamp, 100);
    EXPECT_EQ(scheduler.current_time(), 100);

    auto e2 = scheduler.pop_next();
    ASSERT_TRUE(e2.has_value());
    EXPECT_EQ(e2->timestamp, 200);
    EXPECT_EQ(scheduler.current_time(), 200);

    auto e3 = scheduler.pop_next();
    ASSERT_TRUE(e3.has_value());
    EXPECT_EQ(e3->timestamp, 300);
    EXPECT_EQ(scheduler.current_time(), 300);

    EXPECT_TRUE(scheduler.empty());
}

TEST(SchedulerTest, BreaksTiesDeterministicallyUsingSequence) {
    ghostnet::Scheduler scheduler;

    // Both scheduled at identical virtual timestamp 500
    scheduler.schedule(500, ghostnet::EventType::REQUEST_GENERATED, 1, 2, 1);
    scheduler.schedule(500, ghostnet::EventType::FAULT_ACTIVATED, 2, 2, 2);

    auto e1 = scheduler.pop_next();
    ASSERT_TRUE(e1.has_value());
    EXPECT_EQ(e1->timestamp, 500);
    EXPECT_EQ(e1->type, ghostnet::EventType::REQUEST_GENERATED);

    auto e2 = scheduler.pop_next();
    ASSERT_TRUE(e2.has_value());
    EXPECT_EQ(e2->timestamp, 500);
    EXPECT_EQ(e2->type, ghostnet::EventType::FAULT_ACTIVATED);
}

TEST(SchedulerTest, RejectsEventsScheduledInThePast) {
    ghostnet::Scheduler scheduler;

    scheduler.schedule(200, ghostnet::EventType::REQUEST_GENERATED, 1, 2);
    scheduler.pop_next(); // Advances current_time to 200

    EXPECT_THROW(
        scheduler.schedule(150, ghostnet::EventType::LINK_ARRIVAL, 1, 2),
        std::invalid_argument
    );
}