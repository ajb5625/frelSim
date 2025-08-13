#include "Component.hpp"


namespace frelsim::model {

Matrix Component::jacobian() {
    return Matrix();
}

void Component::defineEvents() {}

std::vector<event::EventIndicator> Component::eventIndicators() {
    std::vector<event::EventIndicator> eventIndicators;
    for (auto& event : events_) {
        eventIndicators.push_back(event.getEventIndicator());
    }
    return eventIndicators;
}
} // frelsim::model
