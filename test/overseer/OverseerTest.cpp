#include <gtest/gtest.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <google/protobuf/util/json_util.h>
#include "frelsim/overseer/Overseer.hpp"

namespace frelsim::overseer {
namespace {

class TempFile {
    public:
        explicit TempFile(std::string const& contents) : path_(nextPath()) {
            std::ofstream file(path_);
            file << contents;
        }

        ~TempFile() {
            std::remove(path_.c_str());
        }

        std::string const& path() const { return path_; }

    private:
        static std::string nextPath() {
            static std::atomic<int> counter{0};
            return (std::filesystem::temp_directory_path()
                / ("frelsim_overseer_test_" + std::to_string(counter++) + ".json")).string();
        }

        std::string path_;
};

// Reserves a unique path without creating the file - for output the test
// itself doesn't write (e.g. a CSV log Overseer is expected to produce).
class TempOutputPath {
    public:
        TempOutputPath() : path_(nextPath()) {}

        ~TempOutputPath() {
            std::remove(path_.c_str());
        }

        std::string const& path() const { return path_; }

        std::string readContents() const {
            std::ifstream file(path_);
            std::ostringstream contents;
            contents << file.rdbuf();
            return contents.str();
        }

    private:
        static std::string nextPath() {
            static std::atomic<int> counter{0};
            return (std::filesystem::temp_directory_path()
                / ("frelsim_overseer_test_log_" + std::to_string(counter++) + ".csv")).string();
        }

        std::string path_;
};

sim::proto::SimulationDescription makePidDescription(double stopTime) {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("PIDController");
    auto* task = desc.mutable_task();
    task->set_task_id(1);
    task->set_task_type(sim::proto::TaskType::PeriodicDiscrete);
    task->set_period(0.05);
    task->set_offset(0.0);
    desc.set_stop_time(stopTime);
    return desc;
}

sim::proto::SimulationDescription makePlantDescription(double stopTime) {
    sim::proto::SimulationDescription desc;
    desc.mutable_model_spec()->set_component_name("MassSpringDamper");
    auto* task = desc.mutable_task();
    task->set_task_id(2);
    task->set_task_type(sim::proto::TaskType::Continuous);
    task->set_period(0.02);
    task->set_max_step_size(0.02);
    desc.set_stop_time(stopTime);
    return desc;
}

sim::proto::Identifier makeId(std::string const& domain, std::string const& scope, std::string const& name) {
    sim::proto::Identifier id;
    id.set_domain(domain);
    id.set_scope(scope);
    id.set_name(name);
    return id;
}

// Same PID-driving-a-plant composition as LinkerTest/SimulatorTest - the
// real, end-to-end entry point this time, exercised through both of
// Overseer's constructors and both of its run modes.
sim::proto::System makeSystem(double stopTime) {
    sim::proto::System system;
    system.set_stop_time(stopTime);
    system.set_max_step_size(0.02);

    auto* pid = system.add_composition();
    pid->mutable_simulation()->set_domain("pid");
    *pid->mutable_sim_description() = makePidDescription(stopTime);
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
    *plant->mutable_sim_description() = makePlantDescription(stopTime);
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

TEST(OverseerTest, SimConstructorRunsToCompletion) {
    Overseer overseer(makeSystem(0.5));
    overseer.initialize();

    overseer.sim();

    EXPECT_NEAR(overseer.simulationTime(), 0.5, 1e-9);
    Values const position = overseer.get("plant", positionOutputIds());
    ASSERT_EQ(position.size(), 1u);
    EXPECT_GT(position[0].asDouble(), 0.0);
}

TEST(OverseerTest, StepConstructorReachesTheSameEndStateAsSim) {
    Overseer stepped(makeSystem(0.5));
    stepped.initialize();
    bool finished = false;
    int steps = 0;
    while (!finished) {
        finished = stepped.step(0.5);
        ASSERT_LT(++steps, 1000) << "step() never reached stop_time";
    }

    Overseer ranToCompletion(makeSystem(0.5));
    ranToCompletion.initialize();
    ranToCompletion.sim();

    EXPECT_NEAR(stepped.simulationTime(), ranToCompletion.simulationTime(), 1e-9);
    Values const steppedPosition = stepped.get("plant", positionOutputIds());
    Values const ranPosition = ranToCompletion.get("plant", positionOutputIds());
    ASSERT_EQ(steppedPosition.size(), 1u);
    ASSERT_EQ(ranPosition.size(), 1u);
    EXPECT_NEAR(steppedPosition[0].asDouble(), ranPosition[0].asDouble(), 1e-9);
}

TEST(OverseerTest, ConfigPathConstructorParsesAndRunsTheSameSystem) {
    sim::proto::System const system = makeSystem(0.3);
    std::string json;
    ASSERT_TRUE(google::protobuf::util::MessageToJsonString(system, &json).ok());
    TempFile const config(json);

    Overseer overseer(config.path());
    overseer.initialize();
    overseer.sim();

    EXPECT_NEAR(overseer.simulationTime(), 0.3, 1e-9);
}

TEST(OverseerTest, MethodsThrowBeforeInitializeIsCalled) {
    Overseer overseer(makeSystem(0.2));

    EXPECT_THROW(overseer.sim(), std::logic_error);
    EXPECT_THROW(overseer.step(0.1), std::logic_error);
    EXPECT_THROW(overseer.simulationTime(), std::logic_error);
}

TEST(OverseerTest, SimAutoWritesTheLogWhenLoggedOutputsAndLogPathAreConfigured) {
    sim::proto::System system = makeSystem(0.3);
    *system.add_logged_outputs() = makeId("plant", "Output", "position");
    *system.add_logged_outputs() = makeId("pid", "Output", "output");

    TempOutputPath const logFile;
    system.set_log_path(logFile.path());

    Overseer overseer(system);
    overseer.initialize();
    overseer.sim();

    std::istringstream contents(logFile.readContents());
    std::string header;
    std::getline(contents, header);
    EXPECT_EQ(header, "time,plant.Output.position,pid.Output.output");

    int rowCount = 0;
    std::string line;
    while (std::getline(contents, line)) {
        ++rowCount;
    }
    EXPECT_GT(rowCount, 0);
}

TEST(OverseerTest, WriteLogWorksExplicitlyAfterAStepDrivenRun) {
    sim::proto::System system = makeSystem(0.2);
    *system.add_logged_outputs() = makeId("plant", "Output", "position");

    Overseer overseer(system);
    overseer.initialize();

    bool finished = false;
    int steps = 0;
    while (!finished) {
        finished = overseer.step(0.2);
        ASSERT_LT(++steps, 1000) << "step() never reached stop_time";
    }

    // step()-driven runs don't auto-write (no log_path set here either) -
    // the caller writes explicitly once it's done.
    TempOutputPath const logFile;
    overseer.writeLog(logFile.path());

    std::istringstream contents(logFile.readContents());
    std::string header;
    std::getline(contents, header);
    EXPECT_EQ(header, "time,plant.Output.position");
}

TEST(OverseerTest, WriteLogThrowsWithoutLoggedOutputsConfigured) {
    Overseer overseer(makeSystem(0.2));
    overseer.initialize();

    TempOutputPath const logFile;
    EXPECT_THROW(overseer.writeLog(logFile.path()), std::logic_error);
}

} // namespace
} // namespace frelsim::overseer
