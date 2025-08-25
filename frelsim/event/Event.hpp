#pragma once
#include <functional>
#include "../util/Aliases.hpp"

namespace frelsim::event {

/**
 * \brief EventIndicator returns g, and accepts time,
 * continuous states, discrete states, and inputs.
 */
using EventIndicator = std::function<double(double, State const&, State const&, Values const&)>;


/**
 * \brief Event handler receives time, continuous states, discrete states, parameters, inputs, and outputs.
 * It is able to mutate all of these.
 */
using EventHandler = std::function<void(
          double currentTime
        , State& continuousStates
        , State& discreteStates
        , Parameters& tunableParameters
        , Values const& inputs
        , Values& outputs)>;


enum class EventType : int {
    Rising = 0,
    Falling = 1, 
    Either = 2
};

class Event final {

    public:
        Event(EventIndicator const& ei, EventHandler const& eh, EventType eventType);

        ~Event() = default;

        EventIndicator getEventIndicator() const;

        EventHandler getEventHandler() const;

        bool firesBetween(double y0, double y1);

        double evaluateIndicatorAt(double simTime
                                , State const& continuousStates
                                , State const& discreteStates
                                , Values const& inputs) const;

        void handleEventAt(double simTime
                        , State& continuousStates
                        , State& discreteStates
                        , Parameters& parameters
                        , Values const& inputs
                        , Values& outputs) const;

    private:

        EventIndicator eventIndicator_;

        EventHandler eventHandler_;

        EventType eventType_;






};


} // frelsim::event 