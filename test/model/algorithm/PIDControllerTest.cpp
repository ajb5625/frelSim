#include <gtest/gtest.h>
#include "frelsim/model/algorithm/PIDController.hpp"

namespace frelsim::model::algorithm {
namespace {

sim::proto::SimulationDescription makePeriodicDescription(double period) {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("PIDController");
    auto* task = desc.mutable_task();
    task->set_task_id(1);
    task->set_task_type(sim::proto::TaskType::PeriodicDiscrete);
    task->set_period(period);
    task->set_offset(0.0); // deliberately 0 - see the regression this guards below
    desc.set_stop_time(1.0);
    return desc;
}

// Regression test for a real bug found running PIDController through
// Simulator (see docs/PROGRESS.md, orchestration track): a periodic task
// with offset=0 sits exactly on its own first scheduled boundary right after
// Model's -1.0 bootstrap step. guaranteeUntil querying the scheduler with
// unnudged internalTime_ reported "now" as the next discrete time forever,
// so simulated time never advanced past t=0 - control()'s integral term wound
// up once per orchestrator call instead of once per sample period.
TEST(PIDControllerTest, GuaranteeUntilAdvancesPastZeroOffsetTaskBoundary) {
    PIDController pid(makePeriodicDescription(0.05));
    pid.initialize();

    // Bootstrap step: Model's internalTime_ starts at -1.0, so the first
    // guaranteeUntil is expected to (correctly) clamp to 0.0.
    double const first = pid.guaranteeUntil(1.0);
    EXPECT_DOUBLE_EQ(first, 0.0);
    pid.stepUntil(first);

    // The bug: this used to also return 0.0 forever, since internalTime_ is
    // now exactly 0.0 and the task's own offset (0.0) sits right on it.
    double const second = pid.guaranteeUntil(1.0);
    EXPECT_NEAR(second, 0.05, 1e-9);
    pid.stepUntil(second);

    double const third = pid.guaranteeUntil(1.0);
    EXPECT_NEAR(third, 0.10, 1e-9);
}

} // namespace
} // namespace frelsim::model::algorithm
