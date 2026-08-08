#include <gtest/gtest.h>
#include "sentinel/core/random_engine.hpp"
#include "sentinel/core/event_scheduler.hpp"
#include <vector>

TEST(RandomEngineTest, DeterminismWithSameSeed) {
    sentinel::core::RandomEngine rng1(1337);
    sentinel::core::RandomEngine rng2(1337);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.get_uint64(0, 100000), rng2.get_uint64(0, 100000));
        EXPECT_DOUBLE_EQ(rng1.get_double(0.0, 1.0), rng2.get_double(0.0, 1.0));
    }
}

TEST(EventSchedulerTest, ChronologicalExecutionOrder) {
    sentinel::core::EventScheduler scheduler;
    std::vector<int> execution_order;

    scheduler.schedule_event(100, "Event 3", [&]() { execution_order.push_back(3); });
    scheduler.schedule_event(10, "Event 1", [&]() { execution_order.push_back(1); });
    scheduler.schedule_event(50, "Event 2", [&]() { execution_order.push_back(2); });

    EXPECT_EQ(scheduler.pending_event_count(), 3u);

    scheduler.step_until(200);

    EXPECT_EQ(execution_order.size(), 3u);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
    EXPECT_EQ(scheduler.current_time_ms(), 200u);
}
