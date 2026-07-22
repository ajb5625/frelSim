#include "Simulation.hpp"
#include "../adapt/SimAdapterFactory.hpp"

namespace frelsim::simulation {

Simulation::Simulation(const sim::proto::SimulationDescription& simDescription) : simDescription_(simDescription) {
    simAdapter_ = adapt::createSimAdapter(simDescription_);
}

Simulation::~Simulation() = default;

double Simulation::guaranteeUntil(double maxTime) {
    return simAdapter_->guaranteeUntil(maxTime);
}

bool Simulation::stepUntil(double stopTime) {
    return simAdapter_->stepUntil(stopTime);
}

Values Simulation::get(const Identifiers& identifiers) const {
    return simAdapter_->get(identifiers);
}

void Simulation::set(SetOperations& setOperations) {
    simAdapter_->set(setOperations);
}

} // frelsim::simulation
