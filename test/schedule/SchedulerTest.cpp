#include <gtest/gtest.h>
#include <algorithm>
#include "frelsim/schedule/Scheduler.hpp"

namespace frelsim::schedule {
namespace {

using task::Task;
using task::TaskId;

TaskList makeTaskList() {
    return TaskList{};
}

TEST(SchedulerTest, ContinuousTaskAlwaysHits) {
    TaskList tasks = makeTaskList();
    tasks.push_back(std::make_unique<Task>(/*taskId=*/1, /*period=*/0.0, /*offset=*/0.0));
    Scheduler scheduler(tasks);

    for (double t : {0.0, 1.0, 100.0}) {
        auto hits = scheduler.taskHits(t);
        EXPECT_NE(std::find(hits.begin(), hits.end(), TaskId{1}), hits.end())
            << "continuous task should hit at t=" << t;
    }
}

TEST(SchedulerTest, PeriodicTaskHitsOnlyAtItsPeriodBoundaries) {
    TaskList tasks = makeTaskList();
    tasks.push_back(std::make_unique<Task>(/*taskId=*/2, /*period=*/0.5, /*offset=*/0.0));
    Scheduler scheduler(tasks);

    for (double t : {0.0, 0.5, 1.0, 1.5}) {
        auto hits = scheduler.taskHits(t);
        EXPECT_NE(std::find(hits.begin(), hits.end(), TaskId{2}), hits.end())
            << "periodic task should hit at t=" << t;
    }
    auto missHits = scheduler.taskHits(0.25);
    EXPECT_EQ(std::find(missHits.begin(), missHits.end(), TaskId{2}), missHits.end());
}

TEST(SchedulerTest, AperiodicTaskHitsOnlyOnceAtItsOffset) {
    TaskList tasks = makeTaskList();
    tasks.push_back(std::make_unique<Task>(/*taskId=*/3, /*period=*/0.0, /*offset=*/2.0));
    Scheduler scheduler(tasks);

    EXPECT_TRUE(scheduler.areAperiodicTasksUpcoming());
    auto [nextId, nextTime] = scheduler.getNextAperiodicTask();
    EXPECT_EQ(nextId, TaskId{3});
    EXPECT_DOUBLE_EQ(nextTime, 2.0);

    auto hitsAtOffset = scheduler.taskHits(2.0);
    EXPECT_NE(std::find(hitsAtOffset.begin(), hitsAtOffset.end(), TaskId{3}), hitsAtOffset.end());

    auto hitsElsewhere = scheduler.taskHits(1.0);
    EXPECT_EQ(std::find(hitsElsewhere.begin(), hitsElsewhere.end(), TaskId{3}), hitsElsewhere.end());
}

TEST(SchedulerTest, ReconcileSchedulerRemovesPassedAperiodicTasks) {
    TaskList tasks = makeTaskList();
    tasks.push_back(std::make_unique<Task>(/*taskId=*/4, /*period=*/0.0, /*offset=*/1.0));
    Scheduler scheduler(tasks);

    ASSERT_TRUE(scheduler.areAperiodicTasksUpcoming());
    scheduler.reconcileScheduler(1.0);
    EXPECT_FALSE(scheduler.areAperiodicTasksUpcoming());
}

TEST(SchedulerTest, GetNextDiscreteTimeFindsNextPeriodicBoundary) {
    TaskList tasks = makeTaskList();
    tasks.push_back(std::make_unique<Task>(/*taskId=*/5, /*period=*/0.5, /*offset=*/0.0));
    Scheduler scheduler(tasks);

    // Just past a boundary at t=1.0, the next hit should be t=1.5.
    EXPECT_DOUBLE_EQ(scheduler.getNextDiscreteTime(1.0 + 1e-6), 1.5);
}

TEST(SchedulerTest, GetNextDiscreteTimeReturnsEarliestOfPeriodicAndAperiodic) {
    TaskList tasks = makeTaskList();
    tasks.push_back(std::make_unique<Task>(/*taskId=*/6, /*period=*/1.0, /*offset=*/0.0));
    tasks.push_back(std::make_unique<Task>(/*taskId=*/7, /*period=*/0.0, /*offset=*/0.3));
    Scheduler scheduler(tasks);

    // Periodic task's next hit from t=0.1 is t=1.0, but the aperiodic task at
    // t=0.3 is earlier, so that should win.
    EXPECT_DOUBLE_EQ(scheduler.getNextDiscreteTime(0.1), 0.3);
}

} // namespace
} // namespace frelsim::schedule
