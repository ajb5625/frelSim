#include "Model.hpp"
#include "../../task/Task.hpp"

namespace frelsim::model::core {

Model::~Model() = default;

JacobianFunction const& Model::jacobian() const {
    static const JacobianFunction none = nullptr;
    return none;
}

std::vector<event::Event> const& Model::events() const {
    static const std::vector<event::Event> none;
    return none;
}

Values Model::getParameters(Identifiers ids) const {
    (void)ids;
    return {};
}

void Model::setParameters(SetOperations ops) {
    (void)ops;
}

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
    // Nudge strictly past internalTime_ before asking for the next discrete
    // time. getNextDiscreteTime treats an exact boundary match as "next",
    // which is what stepUntil's own hit-detection below wants (it queries
    // with its *target* stop time, to check "did this step land exactly on
    // a scheduled hit"). But here internalTime_ is where we already are -
    // if a periodic task's offset lands exactly on it (e.g. offset=0 means
    // the task fires at t=0, and internalTime_ sits at exactly 0.0 right
    // after the bootstrap step), querying with internalTime_ unnudged would
    // report "now" as the next discrete time forever, since nothing else
    // marks that boundary as already consumed - simulated time would never
    // advance past it.
    const double horizon = scheduler_->getNextDiscreteTime(internalTime_ + TinyTolerance);
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