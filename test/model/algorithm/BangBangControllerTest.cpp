#include <gtest/gtest.h>
#include "frelsim/model/algorithm/BangBangController.hpp"
#include "frelsim/util/Identifier.hpp"

namespace frelsim::model::algorithm {
namespace {

sim::proto::SimulationDescription makePeriodicDescription(double period) {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("BangBangController");
    auto* task = desc.mutable_task();
    task->set_task_type(sim::proto::TaskType::PeriodicDiscrete);
    task->set_period(period);
    task->set_offset(0.0);
    desc.set_stop_time(1.0);
    return desc;
}

BangBangController makeController() {
    return BangBangController(makePeriodicDescription(0.1));
}

TEST(BangBangControllerTest, GetOutputsReturnsTypedInitialOutput) {
    BangBangController controller = makeController();
    Values values = controller.getOutputs({util::Identifier("sim.Output.output")});
    ASSERT_EQ(values.size(), 1u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 0.0);
}

TEST(BangBangControllerTest, GetParametersReturnsTypedDefaults) {
    BangBangController controller = makeController();
    Identifiers ids{util::Identifier("sim.Parameter.setpoint"), util::Identifier("sim.Parameter.amplitude")};

    Values values = controller.getParameters(ids);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 0.0);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 1.0);
}

TEST(BangBangControllerTest, SetParametersUpdatesSetpointAndAmplitude) {
    BangBangController controller = makeController();

    SetOperations ops{
        {util::Identifier("sim.Parameter.setpoint"), type::core::Value::makeDouble(10.0)},
        {util::Identifier("sim.Parameter.amplitude"), type::core::Value::makeDouble(5.0)},
    };
    controller.setParameters(ops);

    Identifiers ids{util::Identifier("sim.Parameter.setpoint"), util::Identifier("sim.Parameter.amplitude")};
    Values values = controller.getParameters(ids);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 10.0);
    EXPECT_DOUBLE_EQ(values[1].asDouble(), 5.0);
}

TEST(BangBangControllerTest, SetInputsUpdatesMeasurementReadableViaGetInputs) {
    BangBangController controller = makeController();
    controller.setInputs({{util::Identifier("sim.Input.measurement"), type::core::Value::makeDouble(3.5)}});

    Values values = controller.getInputs({util::Identifier("sim.Input.measurement")});
    ASSERT_EQ(values.size(), 1u);
    EXPECT_DOUBLE_EQ(values[0].asDouble(), 3.5);
}

TEST(BangBangControllerTest, OutputsPositiveAmplitudeWhenBelowSetpoint) {
    BangBangController controller = makeController();
    controller.initialize();
    controller.setParameters({
        {util::Identifier("sim.Parameter.setpoint"), type::core::Value::makeDouble(10.0)},
        {util::Identifier("sim.Parameter.amplitude"), type::core::Value::makeDouble(2.0)},
    });
    controller.setInputs({{util::Identifier("sim.Input.measurement"), type::core::Value::makeDouble(5.0)}});

    // Bootstrap step (see Model.hpp's internalTime_ = -1.0 sentinel), then
    // the first real hit at the task's own period boundary triggers update().
    controller.stepUntil(0.0);
    controller.stepUntil(0.1);

    Values const output = controller.getOutputs({util::Identifier("sim.Output.output")});
    EXPECT_DOUBLE_EQ(output[0].asDouble(), 2.0);
}

TEST(BangBangControllerTest, OutputsNegativeAmplitudeWhenAboveSetpoint) {
    BangBangController controller = makeController();
    controller.initialize();
    controller.setParameters({
        {util::Identifier("sim.Parameter.setpoint"), type::core::Value::makeDouble(10.0)},
        {util::Identifier("sim.Parameter.amplitude"), type::core::Value::makeDouble(2.0)},
    });
    controller.setInputs({{util::Identifier("sim.Input.measurement"), type::core::Value::makeDouble(15.0)}});

    controller.stepUntil(0.0);
    controller.stepUntil(0.1);

    Values const output = controller.getOutputs({util::Identifier("sim.Output.output")});
    EXPECT_DOUBLE_EQ(output[0].asDouble(), -2.0);
}

} // namespace
} // namespace frelsim::model::algorithm
