#include <gtest/gtest.h>
#include <cmath>
#include "frelsim/model/algorithm/FirstOrderLag.hpp"
#include "frelsim/util/Identifier.hpp"

namespace frelsim::model::algorithm {
namespace {

FirstOrderLag makeLag() {
    sim::proto::SimulationDescription desc; // defaults: a valid Continuous task
    return FirstOrderLag(desc);
}

TEST(FirstOrderLagTest, GetOutputsReturnsTypedInitialOutput) {
    FirstOrderLag lag = makeLag();
    Identifiers ids{util::Identifier("sim.Output.output")};

    Values values = lag.getOutputs(ids);
    ASSERT_EQ(values.size(), 1u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 0.0);
}

TEST(FirstOrderLagTest, GetParametersReturnsTypedDefaults) {
    FirstOrderLag lag = makeLag();
    Identifiers ids{util::Identifier("sim.Parameter.timeConstant"), util::Identifier("sim.Parameter.gain")};

    Values values = lag.getParameters(ids);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 1.0);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 1.0);
}

TEST(FirstOrderLagTest, SetParametersUpdatesTimeConstantAndGain) {
    FirstOrderLag lag = makeLag();

    SetOperations ops{
        {util::Identifier("sim.Parameter.timeConstant"), type::core::Value::makeDouble(2.0)},
        {util::Identifier("sim.Parameter.gain"), type::core::Value::makeDouble(3.0)},
    };
    lag.setParameters(ops);

    Identifiers ids{util::Identifier("sim.Parameter.timeConstant"), util::Identifier("sim.Parameter.gain")};
    Values values = lag.getParameters(ids);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 2.0);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 3.0);
}

TEST(FirstOrderLagTest, SetInputsUpdatesInputReadableViaGetInputs) {
    FirstOrderLag lag = makeLag();
    SetOperations ops{{util::Identifier("sim.Input.input"), type::core::Value::makeDouble(5.0)}};
    lag.setInputs(ops);

    Values values = lag.getInputs({util::Identifier("sim.Input.input")});
    ASSERT_EQ(values.size(), 1u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 5.0);
}

sim::proto::SimulationDescription makeSteppableDescription(double timeConstant, double gain, double stopTime) {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("FirstOrderLag");
    desc.mutable_model_spec()->set_solver_type(sim::proto::SolverType::DormandPrince);
    auto* task = desc.mutable_task();
    task->set_task_type(sim::proto::TaskType::Continuous);
    task->set_max_step_size(stopTime / 10.0);
    desc.set_stop_time(stopTime);
    (void)timeConstant;
    (void)gain;
    return desc;
}

TEST(FirstOrderLagTest, MatchesClosedFormStepResponse) {
    // A held step input u=1 from a zero initial condition has the classic
    // closed-form solution y(t) = K*u*(1 - e^(-t/tau)) - this is exactly
    // the textbook first-order-lag step response.
    double const timeConstant = 0.5;
    double const gain = 2.0;
    double const stopTime = 1.0;

    FirstOrderLag lag(makeSteppableDescription(timeConstant, gain, stopTime));
    lag.setParameters({
        {util::Identifier("sim.Parameter.timeConstant"), type::core::Value::makeDouble(timeConstant)},
        {util::Identifier("sim.Parameter.gain"), type::core::Value::makeDouble(gain)},
    });
    lag.initialize();
    lag.setInputs({{util::Identifier("sim.Input.input"), type::core::Value::makeDouble(1.0)}});

    // internalTime_ starts at -1.0 (see Model.hpp) - the first stepUntil
    // call is a zero-length bootstrap to t=0 rather than a real
    // integration; only the second call actually covers [0, stopTime].
    lag.stepUntil(0.0);
    lag.stepUntil(stopTime);

    double const expected = gain * (1.0 - std::exp(-stopTime / timeConstant));
    Values const output = lag.getOutputs({util::Identifier("sim.Output.output")});
    EXPECT_NEAR(output[0].asDouble(), expected, 1e-6);
}

} // namespace
} // namespace frelsim::model::algorithm
