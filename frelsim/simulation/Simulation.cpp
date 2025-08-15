#include "Simulation.hpp"


namespace frelsim::simulation {

void Simulation::parameters() {}

void Simulation::events() {}

std::vector<event::EventIndicator> Simulation::eventIndicators() const {
    std::vector<event::EventIndicator> eventIndicators;
    for (auto& event : events_) {
        eventIndicators.push_back(event.getEventIndicator());
    }
    return eventIndicators;
}
} // frelsim::simulation
