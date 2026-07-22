#pragma once
#include <functional>
#include "../util/Aliases.hpp"

namespace frelsim::event {

/**
 * \brief EventIndicator returns g(elapsed, continuousStates, discreteStates, inputs);
 * an Event fires when g crosses zero (per its EventType).
 *
 * IMPORTANT: the first argument is elapsed time SINCE the (continuousStates,
 * discreteStates) snapshot given to it, not the absolute simulation clock.
 * EventEngine's lookahead search (nextEventTime) evaluates an indicator many
 * times against one fixed anchor state without re-integrating the model in
 * between (that would mean re-running the solver at every probe, which is far
 * too expensive to do just to look for a crossing) - so the only thing that
 * varies between calls is how far past the anchor we're asking it to predict.
 * A correct indicator therefore needs a closed-form prediction of g at a given
 * elapsed time, e.g. a free-falling body's height is
 * h0 + v0*elapsed - 0.5*g*elapsed^2, where h0/v0 come from continuousStates.
 * When called after a real integration step (elapsed == 0), it should reduce
 * to just reading the current state, which the same formula does naturally.
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

        /**
         * \brief Evaluate this event's indicator. `elapsed` is elapsed time
         * since the given (continuousStates, discreteStates) snapshot was
         * captured, not the absolute simulation clock - see the EventIndicator
         * comment above.
         */
        double evaluateIndicatorAt(double elapsed
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