#include "Simulation.hpp"


namespace frelsim::simulation {

Simulation::Simulation(const sim::proto::SimulationDescription& simDescription) : simDescription_(simDescription) {
    
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

// std::vector<event::EventIndicator> Simulation::eventIndicators() const {
//     std::vector<event::EventIndicator> eventIndicators;
//     for (auto& event : events_) {
//         eventIndicators.push_back(event.getEventIndicator());
//     }
//     return eventIndicators;
// }
} // frelsim::simulation
