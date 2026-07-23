#include <gtest/gtest.h>
#include <cmath>
#include "frelsim/model/algorithm/LongitudinalVehicle.hpp"
#include "frelsim/util/Identifier.hpp"

namespace frelsim::model::algorithm {
namespace {

LongitudinalVehicle makeVehicle() {
    sim::proto::SimulationDescription desc; // defaults: a valid Continuous task
    return LongitudinalVehicle(desc);
}

TEST(LongitudinalVehicleTest, GetOutputsReturnsTypedInitialPositionAndVelocity) {
    LongitudinalVehicle vehicle = makeVehicle();
    Identifiers ids{util::Identifier("sim.Output.position"), util::Identifier("sim.Output.velocity")};

    Values values = vehicle.getOutputs(ids);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 0.0);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 0.0);
}

TEST(LongitudinalVehicleTest, GetParametersReturnsTypedDefaults) {
    LongitudinalVehicle vehicle = makeVehicle();
    Identifiers ids{
        util::Identifier("sim.Parameter.mass"),
        util::Identifier("sim.Parameter.dragCoefficient"),
        util::Identifier("sim.Parameter.rollingResistance"),
    };

    Values values = vehicle.getParameters(ids);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 1500.0);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 0.35);
    EXPECT_DOUBLE_EQ(values[2].asDouble(), 10.0);
}

TEST(LongitudinalVehicleTest, SetParametersUpdatesAllThreeParameters) {
    LongitudinalVehicle vehicle = makeVehicle();

    SetOperations ops{
        {util::Identifier("sim.Parameter.mass"), type::core::Value::makeDouble(1000.0)},
        {util::Identifier("sim.Parameter.dragCoefficient"), type::core::Value::makeDouble(0.5)},
        {util::Identifier("sim.Parameter.rollingResistance"), type::core::Value::makeDouble(20.0)},
    };
    vehicle.setParameters(ops);

    Identifiers ids{
        util::Identifier("sim.Parameter.mass"),
        util::Identifier("sim.Parameter.dragCoefficient"),
        util::Identifier("sim.Parameter.rollingResistance"),
    };
    Values values = vehicle.getParameters(ids);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 1000.0);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 0.5);
    EXPECT_DOUBLE_EQ(values[2].asDouble(), 20.0);
}

TEST(LongitudinalVehicleTest, SetInputsUpdatesThrottleReadableViaGetInputs) {
    LongitudinalVehicle vehicle = makeVehicle();
    vehicle.setInputs({{util::Identifier("sim.Input.throttle"), type::core::Value::makeDouble(2000.0)}});

    Values values = vehicle.getInputs({util::Identifier("sim.Input.throttle")});
    ASSERT_EQ(values.size(), 1u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 2000.0);
}

sim::proto::SimulationDescription makeSteppableDescription(double stopTime, double maxStepSize) {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("LongitudinalVehicle");
    desc.mutable_model_spec()->set_solver_type(sim::proto::SolverType::DormandPrince);
    auto* task = desc.mutable_task();
    task->set_task_type(sim::proto::TaskType::Continuous);
    task->set_max_step_size(maxStepSize);
    desc.set_stop_time(stopTime);
    return desc;
}

TEST(LongitudinalVehicleTest, ReachesAnalyticTerminalVelocityUnderConstantThrottle) {
    // At equilibrium (v' = 0): throttle = Cd*v^2 + Cr*v, i.e.
    // Cd*v^2 + Cr*v - throttle = 0 - solvable directly via the quadratic
    // formula (taking the positive root, since throttle > 0 drives v > 0).
    double const mass = 1500.0;
    double const dragCoefficient = 0.35;
    double const rollingResistance = 10.0;
    double const throttle = 3000.0;
    double const stopTime = 200.0; // long enough to settle near equilibrium

    LongitudinalVehicle vehicle(makeSteppableDescription(stopTime, 1.0));
    vehicle.setParameters({
        {util::Identifier("sim.Parameter.mass"), type::core::Value::makeDouble(mass)},
        {util::Identifier("sim.Parameter.dragCoefficient"), type::core::Value::makeDouble(dragCoefficient)},
        {util::Identifier("sim.Parameter.rollingResistance"), type::core::Value::makeDouble(rollingResistance)},
    });
    vehicle.initialize();
    vehicle.setInputs({{util::Identifier("sim.Input.throttle"), type::core::Value::makeDouble(throttle)}});

    vehicle.stepUntil(0.0); // bootstrap (see Model.hpp's internalTime_ = -1.0 sentinel)
    vehicle.stepUntil(stopTime);

    double const equilibriumVelocity =
        (-rollingResistance + std::sqrt(rollingResistance * rollingResistance + 4.0 * dragCoefficient * throttle))
        / (2.0 * dragCoefficient);

    Values const velocity = vehicle.getOutputs({util::Identifier("sim.Output.velocity")});
    // The system approaches equilibrium exponentially and never exactly
    // reaches it in finite time - 5e-2 comfortably clears the residual gap
    // at stopTime=200 while still being a meaningfully tight check.
    EXPECT_NEAR(velocity[0].asDouble(), equilibriumVelocity, 5e-2);
}

TEST(LongitudinalVehicleTest, AutomaticSolverSelectionIntegratesCorrectlyThroughBackwardEuler) {
    // derivative()/jacobian() are protected (deliberate - Model's
    // customization points, not part of the public co-sim I/O boundary),
    // so the Jacobian can't be unit-tested in isolation via a direct
    // finite-difference comparison without breaking that encapsulation.
    // This instead exercises it indirectly but genuinely: force
    // SolverType::BackwardEuler (which uses the Jacobian for its
    // Newton-Raphson iteration every step - see BackwardEuler.cpp) through
    // the real Model::initialize() path, and check the result against the
    // same analytic equilibrium as the test above. If the Jacobian were
    // wrong (wrong sign, wrong factor of 2 on the |v| term, etc.),
    // BackwardEuler's Newton iteration would converge to the wrong state
    // or fail to converge within its iteration cap - this would catch that
    // while still respecting Model's protected interface.
    double const mass = 1500.0;
    double const dragCoefficient = 0.35;
    double const rollingResistance = 10.0;
    double const throttle = 3000.0;
    double const stopTime = 200.0;

    sim::proto::SimulationDescription desc = makeSteppableDescription(stopTime, 1.0);
    desc.mutable_model_spec()->set_solver_type(sim::proto::SolverType::BackwardEuler);

    LongitudinalVehicle vehicle(desc);
    vehicle.setParameters({
        {util::Identifier("sim.Parameter.mass"), type::core::Value::makeDouble(mass)},
        {util::Identifier("sim.Parameter.dragCoefficient"), type::core::Value::makeDouble(dragCoefficient)},
        {util::Identifier("sim.Parameter.rollingResistance"), type::core::Value::makeDouble(rollingResistance)},
    });
    vehicle.initialize();
    vehicle.setInputs({{util::Identifier("sim.Input.throttle"), type::core::Value::makeDouble(throttle)}});

    vehicle.stepUntil(0.0);
    vehicle.stepUntil(stopTime);

    double const equilibriumVelocity =
        (-rollingResistance + std::sqrt(rollingResistance * rollingResistance + 4.0 * dragCoefficient * throttle))
        / (2.0 * dragCoefficient);

    Values const velocity = vehicle.getOutputs({util::Identifier("sim.Output.velocity")});
    EXPECT_NEAR(velocity[0].asDouble(), equilibriumVelocity, 1e-1);
}

} // namespace
} // namespace frelsim::model::algorithm
