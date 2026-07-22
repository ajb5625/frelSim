#include <gtest/gtest.h>
#include "frelsim/model/algorithm/BouncingBall.hpp"
#include "frelsim/util/Identifier.hpp"

namespace frelsim::model::algorithm {
namespace {

// These exercise BouncingBall's typed getOutputs/getParameters/setParameters
// contract specifically (Type System Stage 2: these used to pass/return raw
// double, now they pass/return frelsim::type::core::Value). Separate from
// the physics itself (event-crossing/bounce behavior is covered by
// EventEngineTest and was manually verified end-to-end via a driver program
// - see docs/PROGRESS.md).

BouncingBall makeBall() {
    sim::proto::SimulationDescription desc; // defaults: a valid Continuous task
    return BouncingBall(desc);
}

TEST(BouncingBallTest, GetOutputsReturnsTypedInitialHeightAndVelocity) {
    BouncingBall ball = makeBall();
    Identifiers ids{util::Identifier("sim.Output.height"), util::Identifier("sim.Output.velocity")};

    Values values = ball.getOutputs(ids);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 10.0); // initial height
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 0.0);  // initial velocity
}

TEST(BouncingBallTest, GetOutputsFallsBackToZeroForUnknownIdentifier) {
    BouncingBall ball = makeBall();
    Identifiers ids{util::Identifier("sim.Output.unknown")};

    Values values = ball.getOutputs(ids);
    ASSERT_EQ(values.size(), 1u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 0.0);
}

TEST(BouncingBallTest, GetParametersReturnsTypedDefaults) {
    BouncingBall ball = makeBall();
    Identifiers ids{util::Identifier("sim.Parameter.gravity"), util::Identifier("sim.Parameter.restitution")};

    Values values = ball.getParameters(ids);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 9.81);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 0.8);
}

TEST(BouncingBallTest, SetParametersUpdatesGravityAndRestitutionFromTypedValues) {
    BouncingBall ball = makeBall();

    SetOperations ops{
        {util::Identifier("sim.Parameter.gravity"), type::core::Value::makeDouble(1.62)},   // lunar gravity
        {util::Identifier("sim.Parameter.restitution"), type::core::Value::makeDouble(0.5)},
    };
    ball.setParameters(ops);

    Identifiers ids{util::Identifier("sim.Parameter.gravity"), util::Identifier("sim.Parameter.restitution")};
    Values values = ball.getParameters(ids);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 1.62);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 0.5);
}

TEST(BouncingBallTest, SetInputsIsANoOpSinceBouncingBallIsAutonomous) {
    BouncingBall ball = makeBall();
    SetOperations ops{{util::Identifier("sim.Input.whatever"), type::core::Value::makeDouble(42.0)}};
    EXPECT_NO_THROW(ball.setInputs(ops));
}

} // namespace
} // namespace frelsim::model::algorithm
