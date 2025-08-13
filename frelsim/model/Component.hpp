#include <functional>
#include <vector>
#include <map>

#include "../event/Event.hpp"

namespace frelsim::model {


using State = std::vector<double>;

using Derivative = std::function<State(const State&, double)>;

using Matrix = std::vector<State>;

class Component {

    public:

        Component() = default;

        virtual ~Component();

        virtual Derivative derivative() = 0;

        virtual Matrix jacobian();

        virtual void defineEvents();

        std::vector<event::EventIndicator> eventIndicators();


    private:
        std::vector<event::Event> events_;        



};


} // frelsim::model 