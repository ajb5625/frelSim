#include <functional>

namespace frelsim::event {

using EventIndicator = std::function<double(double, std::vector<double>)>;


enum class EventType : int {
    Rising = 0,
    Falling = 1, 
    Either = 2
};

class Event {

    public:
        Event(EventType eventType);

        virtual ~Event() = default;

        virtual EventIndicator getEventIndicator();

        bool didEventFire(double y0, double y1);

    private:

        EventIndicator eventIndicator_;

        EventType eventType_;






};


} // frelsim::event 