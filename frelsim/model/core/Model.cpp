#include "Model.hpp"
#include "../../task/Task.hpp"

namespace frelsim::model::core {

Model::Model(const sim::proto::SimulationDescription& simDescription) : simDescription_(simDescription)
                                                                        , stepSize_(simDescription_.task().period())
                                                                        , stopTime_(simDescription_.stop_time()) {
    /**
     * Create the scheduler based on the task we have been given.
     * For now, assume one task per model.
     */
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
    // Create the solver and event engine.
    solver_ = integrate::factory::createSolver(simDescription_.model_spec().solver_type()
                                              , simDescription_.stop_time()
                                              , solverStepSize
                                              , derivative()
                                              , jacobian());
    eventEngine_ = std::make_unique<event::EventEngine>(events()
                                                        , SolverTolerance
                                                        , solverStepSize);
}

double Model::guaranteeUntil(double maxTime) {
    // Find next discrete time scheduled
    const double horizon = scheduler_->getNextDiscreteTime(internalTime_);
    // Find earliest zero crossing
    const double zc = eventEngine_->nextEventTime(internalTime_
                                                , maxTime
                                                , continuousStates_
                                                , discreteStates_
                                                , inputs_);
    return std::min(std::min(horizon, zc), maxTime);
}

bool Model::stepUntil(double stopTime) {
    // Continuous states are modified in place
    solver_->step(continuousStates_, stopTime);
    // resolve zero crossing events
    eventEngine_->processEventsAt(stopTime
                                , continuousStates_
                                , discreteStates_
                                , parameters_
                                , inputs_
                                , outputs_);
    // detect if we have a hit for discrete update
    if (util::almostEqual(scheduler_->getNextDiscreteTime(stopTime), stopTime)) {
        update();
    }
    internalTime_ = stopTime;
    return util::almostEqual(internalTime_, stopTime_);
}

} // frelsim::model::core