#include "Component.hpp"


namespace frelsim::model {

void Component::parameters() {}

void Component::events() {}

std::vector<event::EventIndicator> Component::eventIndicators() const {
    std::vector<event::EventIndicator> eventIndicators;
    for (auto& event : events_) {
        eventIndicators.push_back(event.getEventIndicator());
    }
    return eventIndicators;
}
} // frelsim::model
