#include "Model.hpp"
#include "../../task/Task.hpp"

namespace frelsim::model::core {

Model::Model(const sim::proto::SimulationDescription& simDescription) : simDescription_(simDescription)
                                                                        , stepSize_(simDescription_.task().period())
                                                                        , stopTime_(simDescription_.stop_time()) {
    schedule::TaskList tasklist;
    tasklist.emplace_back(task::deserialize(simDescription_.task()));
    scheduler_ = std::make_unique<schedule::Scheduler>(tasklist);
}

void Model::initialize() {
    auto task = simDescription_.task();
    double solverStepSize = task.period();
    if (task.has_max_step_size()) {
        solverStepSize = task.max_step_size();
    }
    solver_ = integrate::factory::createSolver(simDescription_.model_spec().solver_type()
                                              , simDescription_.stop_time()
                                              , solverStepSize
                                              , derivative()
                                              , jacobian());
}

bool Model::stepUntil(double stopTime) {
    // Continuous states are modified in place
    solver_->step(continuousStates_, stopTime);

    // detect if we have a hit for discrete update
    if (util::almostEqual(scheduler_->getNextDiscreteTime(stopTime), stopTime)) {
        update();
    }
    return util::almostEqual(stopTime, stopTime_);
}

} // frelsim::model::core