#include <gtest/gtest.h>
#include "frelsim/linker/Linker.hpp"

namespace frelsim::linker {
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

// ModelAdapter::get/set dispatch to getOutputs/setInputs/getParameters/
// setParameters purely by an identifier's scope ("Output"/"Input"/
// "Parameter") - an identifier with no scope set is silently dropped by
// both, not an error, so every identifier here needs the right one.
sim::proto::Identifier makeId(std::string const& domain, std::string const& scope, std::string const& name) {
    sim::proto::Identifier id;
    id.set_domain(domain);
    id.set_scope(scope);
    id.set_name(name);
    return id;
}

// A minimal, correctly-wired two-component composition (PID driving a
// plant, plant feeding back into PID's measurement) - see
// Simulator.cpp/Simulator::route() and docs/PROGRESS.md (orchestration
// track) for why the source/destinations on any one RoutedSimulation entry
// aren't tied to that entry's own `simulation` field.
sim::proto::System makeValidSystem() {
    sim::proto::System system;
    system.set_stop_time(1.0);
    system.set_max_step_size(0.02);

    auto* pid = system.add_composition();
    pid->mutable_simulation()->set_domain("pid");
    *pid->mutable_sim_description() = makePidDescription();
    *pid->mutable_source() = makeId("plant", "Output", "position");
    *pid->add_destinations() = makeId("pid", "Input", "measurement");

    auto* plant = system.add_composition();
    plant->mutable_simulation()->set_domain("plant");
    *plant->mutable_sim_description() = makePlantDescription();
    *plant->mutable_source() = makeId("pid", "Output", "output");
    *plant->add_destinations() = makeId("plant", "Input", "force");

    return system;
}

TEST(LinkerTest, LinksAValidCompositionAndReturnsConstructedParameterizedSimulations) {
    sim::proto::System system = makeValidSystem();
    auto* setpointOp = system.mutable_composition(0)->add_initial_parameters();
    setpointOp->mutable_id()->set_scope("Parameter");
    setpointOp->mutable_id()->set_name("setpoint");
    setpointOp->mutable_value()->mutable_type()->mutable_float_type()->set_precision(64);
    // 5.0 as raw little-endian double bytes - matches Marshaler's
    // FloatType(precision=64) encoding used elsewhere in this codebase.
    double const setpoint = 5.0;
    setpointOp->mutable_value()->mutable_data()->assign(
        reinterpret_cast<char const*>(&setpoint), sizeof(setpoint));

    LinkedSystem const linked = Linker().link(system);

    ASSERT_EQ(linked.simulations.size(), 2u);
    ASSERT_TRUE(linked.simulations.count("pid"));
    ASSERT_TRUE(linked.simulations.count("plant"));

    // The initial_parameters application actually took effect.
    util::Identifier setpointId;
    setpointId.setScope("Parameter");
    setpointId.setName("setpoint");
    Values const setpointValue = linked.simulations.at("pid")->get({setpointId});
    ASSERT_EQ(setpointValue.size(), 1u);
    EXPECT_DOUBLE_EQ(setpointValue[0].asDouble(), 5.0);
}

TEST(LinkerTest, ThrowsOnAnUnknownSourceSimulation) {
    sim::proto::System system = makeValidSystem();
    system.mutable_composition(0)->mutable_source()->set_domain("doesNotExist");

    EXPECT_THROW(Linker().link(system), std::invalid_argument);
}

TEST(LinkerTest, ThrowsOnAnUnknownDestinationSimulation) {
    sim::proto::System system = makeValidSystem();
    system.mutable_composition(0)->mutable_destinations(0)->set_domain("doesNotExist");

    EXPECT_THROW(Linker().link(system), std::invalid_argument);
}

} // namespace
} // namespace frelsim::linker
