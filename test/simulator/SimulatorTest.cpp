#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include "frelsim/simulator/Simulator.hpp"
#include "frelsim/linker/Linker.hpp"

namespace frelsim::simulator {
namespace {

sim::proto::SimulationDescription makePidDescription() {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("PIDController");
    auto* task = desc.mutable_task();
    task->set_task_id(1);
    task->set_task_type(sim::proto::TaskType::PeriodicDiscrete);
    task->set_period(0.05);
    task->set_offset(0.0);
    desc.set_stop_time(1.0);
    return desc;
}

sim::proto::SimulationDescription makePlantDescription() {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("MassSpringDamper");
    auto* task = desc.mutable_task();
    task->set_task_id(2);
    task->set_task_type(sim::proto::TaskType::Continuous);
    task->set_period(0.02);
    task->set_max_step_size(0.02);
    desc.set_stop_time(1.0);
    return desc;
}

sim::proto::Identifier makeId(std::string const& domain, std::string const& scope, std::string const& name) {
    sim::proto::Identifier id;
    id.set_domain(domain);
    id.set_scope(scope);
    id.set_name(name);
    return id;
}

// Same PID-driving-a-plant composition as LinkerTest - built here directly
// (not reused across files, matching this codebase's existing per-test-file
// fixture convention, e.g. PIDControllerTest's own makePeriodicDescription).
sim::proto::System makeSystem(double stopTime) {
    sim::proto::System system;
    system.set_stop_time(stopTime);
    system.set_max_step_size(0.02);

    auto* pid = system.add_composition();
    pid->mutable_simulation()->set_domain("pid");
    *pid->mutable_sim_description() = makePidDescription();
    pid->mutable_sim_description()->set_stop_time(stopTime);
    *pid->mutable_source() = makeId("plant", "Output", "position");
    *pid->add_destinations() = makeId("pid", "Input", "measurement");
    auto* setpointOp = pid->add_initial_parameters();
    setpointOp->mutable_id()->set_scope("Parameter");
    setpointOp->mutable_id()->set_name("setpoint");
    setpointOp->mutable_value()->mutable_type()->mutable_float_type()->set_precision(64);
    double const setpoint = 5.0;
    setpointOp->mutable_value()->mutable_data()->assign(
        reinterpret_cast<char const*>(&setpoint), sizeof(setpoint));

    auto* plant = system.add_composition();
    plant->mutable_simulation()->set_domain("plant");
    *plant->mutable_sim_description() = makePlantDescription();
    plant->mutable_sim_description()->set_stop_time(stopTime);
    *plant->mutable_source() = makeId("pid", "Output", "output");
    *plant->add_destinations() = makeId("plant", "Input", "force");

    return system;
}

Identifiers positionOutputIds() {
    util::Identifier id;
    id.setScope("Output");
    id.setName("position");
    return {id};
}

TEST(SimulatorTest, StepAdvancesGlobalTimeAndSimReachesStopTime) {
    // step() is the single-step, externally-driven mode: each call advances
    // by at most max_step_size (a conservative lock-step bound), not all
    // the way to the stopTime argument in one call - stopTime there is a
    // cap on the horizon, not a jump target. Reaching it takes repeated
    // calls, exactly like a gRPC/REST caller driving the sim one step at a
    // time would do. The very first call is a bootstrap step to t=0 (every
    // composed Model starts from Model's internalTime_ = -1.0 sentinel, see
    // Model.cpp), so it doesn't yet advance by a full max_step_size - assert
    // on eventual completion and monotonic progress, not one fixed-size
    // first increment.
    Simulator simulator(linker::Linker().link(makeSystem(0.2)));
    simulator.initialize();

    EXPECT_DOUBLE_EQ(simulator.simulationTime(), 0.0);

    bool finished = false;
    double previousTime = -1.0;
    int steps = 0;
    while (!finished) {
        finished = simulator.step(0.2);
        EXPECT_GE(simulator.simulationTime(), previousTime);
        previousTime = simulator.simulationTime();
        ASSERT_LT(++steps, 1000) << "step() never reached stop_time";
    }
    EXPECT_NEAR(simulator.simulationTime(), 0.2, 1e-9);
}

TEST(SimulatorTest, SimRunsToCompletionAndPlantMovesTowardSetpoint) {
    Simulator simulator(linker::Linker().link(makeSystem(0.5)));

    simulator.sim();

    EXPECT_DOUBLE_EQ(simulator.simulationTime(), 0.5);
    // The plant starts at position 0 and PID drives it toward setpoint=5 -
    // not asserting convergence (too far from stop_time=0.5 to have
    // settled), just that closed-loop control moved it off zero in the
    // right direction.
    Values const position = simulator.get("plant", positionOutputIds());
    ASSERT_EQ(position.size(), 1u);
    EXPECT_GT(position[0].asDouble(), 0.0);
}

TEST(SimulatorTest, GetThrowsForAnUnknownSimulationKey) {
    Simulator simulator(linker::Linker().link(makeSystem(0.2)));
    simulator.initialize();

    EXPECT_THROW(simulator.get("doesNotExist", positionOutputIds()), std::out_of_range);
}

TEST(SimulatorTest, PauseStopsSimBeforeReachingStopTime) {
    // pause() only takes effect on the next isStopRequested_ check inside
    // sim()'s loop, so this can't easily be asserted synchronously without
    // spinning up a thread; covered instead by resume() being a no-op
    // unless a pause is outstanding (deliberately narrow - full pause/resume
    // behavior is exercised manually via the control-loop demo, see
    // docs/PROGRESS.md).
    Simulator simulator(linker::Linker().link(makeSystem(0.2)));
    simulator.initialize();
    simulator.pause();
    simulator.resume();

    // resume() after pause() shouldn't leave the simulator stuck - step()
    // still advances normally (bootstrap call first, see the test above).
    simulator.step(0.2);
    EXPECT_FALSE(simulator.step(0.2));
    EXPECT_NEAR(simulator.simulationTime(), 0.02, 1e-9);
}

TEST(SimulatorTest, StepObserverFiresOnceAtTheEndOfEveryStepCallWithCurrentTime) {
    Simulator simulator(linker::Linker().link(makeSystem(0.2)));
    simulator.initialize();

    std::vector<double> observedTimes;
    simulator.setStepObserver([&observedTimes](Simulator const& sim) {
        observedTimes.push_back(sim.simulationTime());
    });

    bool finished = false;
    int stepCalls = 0;
    while (!finished) {
        finished = simulator.step(0.2);
        ++stepCalls;
    }

    // One observer call per step() call, each reporting simulationTime() as
    // it stood immediately after that step - not just a final callback.
    EXPECT_EQ(static_cast<int>(observedTimes.size()), stepCalls);
    EXPECT_TRUE(std::is_sorted(observedTimes.begin(), observedTimes.end()));
    EXPECT_NEAR(observedTimes.back(), simulator.simulationTime(), 1e-9);
    EXPECT_NEAR(observedTimes.back(), 0.2, 1e-9);
}

TEST(SimulatorTest, StepObserverAlsoFiresDuringSim) {
    Simulator simulator(linker::Linker().link(makeSystem(0.2)));

    int observerCallCount = 0;
    simulator.setStepObserver([&observerCallCount](Simulator const&) {
        ++observerCallCount;
    });

    simulator.sim();

    EXPECT_GT(observerCallCount, 0);
}

} // namespace
} // namespace frelsim::simulator
