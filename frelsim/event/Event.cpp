#include "Event.hpp"

namespace frelsim::event {

Event::Event(EventType eventType) : eventType_(eventType) {}


bool Event::didEventFire(double y0, double y1) {
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
} // namespace frelsim::event
