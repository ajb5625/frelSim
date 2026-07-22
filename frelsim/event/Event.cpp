#include "Event.hpp"

namespace frelsim::event {

Event::Event(EventIndicator const& ei
            , EventHandler const& eh
            , EventType eventType) : eventIndicator_(ei)
                                    , eventHandler_(eh)
                                    , eventType_(eventType) {}

EventIndicator Event::getEventIndicator() const {
    return eventIndicator_;
}

EventHandler Event::getEventHandler() const {
    return eventHandler_;
}

bool Event::firesBetween(double y0, double y1) {
    switch (eventType_) {
        case EventType::Rising:
            return (y1 >= 0 && y0 < 0);
        break;
        case EventType::Falling:
            return (y1 <= 0 && y0 > 0);
        break;
        case EventType::Either:
            return (y0 * y1 <= 0);
        break;
    }
    return false;
}

double Event::evaluateIndicatorAt(double elapsed
                                , State const& continuousStates
                                , State const& discreteStates
                                , Values const& inputs) const {
    return eventIndicator_(elapsed, continuousStates, discreteStates, inputs);
}

void Event::handleEventAt(double simTime
                        , State& continuousStates
                        , State& discreteStates
                        , Parameters& parameters
                        , Values const& inputs
                        , Values& outputs) const {
    eventHandler_(simTime, continuousStates, discreteStates, parameters, inputs, outputs);
}


} // namespace frelsim::event
