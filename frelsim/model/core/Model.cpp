#include "Model.hpp"

namespace frelsim::model::core {

Model::Model(const sim::proto::SimulationDescription& simDescription) : simDescription_(simDescription)
                                                                        , stepSize_(simDescription_.communication_step_size())
                                                                        , stopTime_(simDescription_.stop_time()) {
    
}

void Model::initialize() {
    solver_ = integrate::factory::createSolver(simDescription_.model_spec().solver_type()
                                              ,simDescription_.stop_time()
                                              ,simDescription_.communication_step_size()
                                              , derivative()
                                              , jacobian());
}

} // frelsim::model::core