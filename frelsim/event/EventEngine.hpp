#pragma once
#include <optional>
#include <vector>
#include "Event.hpp"

namespace frelsim::event {

/**
 * \file EventEngine.hpp
 * \brief The Event engine is responsible for telling a Model when an event has or will occur.
 * It can also operate on the model's states, outputs, and parameters if an event has fired.
 * It keeps track of pending events to be resolved during the model stepUntil call.
 *
 * \note On the meaning of "time" passed to EventIndicator functions:
 * nextEventTime() looks ahead from a single anchor state snapshot (the state at t0) without
 * re-integrating the model at every candidate instant it probes - re-running the solver at
 * every search point would be far too expensive. That means an EventIndicator can only be
 * evaluated correctly if it treats its own time argument as *elapsed time since the anchor
 * state* (t - t0), and predicts forward analytically from (continuousStates, discreteStates)
 * using that elapsed time - e.g. free-fall height as h0 + v0*elapsed - 0.5*g*elapsed^2. It is
 * NOT the absolute simulation time. EventEngine enforces this by always converting absolute
 * times to elapsed-since-anchor before calling into an indicator; see nextEventTime()/
 * bisectRoot()/zerosAt() in EventEngine.cpp for exactly where that conversion happens.
 */

class EventEngine final {

    public:
        EventEngine(std::vector<Event>const& events,
                    double tolerance,
                    double maxSearchStep);

        ~EventEngine() = default;

        // Don't allow copy or copy assignment
        EventEngine(EventEngine const& other) = delete;
        EventEngine& operator=(EventEngine const& other) = delete;

        /**
         * \brief From time t0 to maxTime, find the next zero crossing event time.
         * The states and inputs are needed to evaluate the event indicators.
         */
        double nextEventTime(double t0
                            , double maxTime
                            , State const& continuousStates
                            , State const& discreteStates
                            , Values const& inputs);

        /**
         * \brief At currentTime, process all pending events.
         */
        bool processEventsAt(double currentTime
                            , State& continuousStates
                            , State& discreteStates
                            , Parameters& params
                            , Values const& inputs
                            , Values& outputs);


    private:
        /**
         * \brief Clear the pending event set.
         */
        void clearPending();

        /**
         * \brief Evaluate all event indicators at `elapsed` time since the anchor
         * state (continuousStates/discreteStates), i.e. indicator(elapsed, ...).
         */
        std::vector<double> evaluateIndicators(double elapsed
                                            , State const& continuousStates
                                            , State const& discreteStates
                                            , Values const& inputs) const;

        /**
         * \brief Evaluate the indicator corresponding to idx at `elapsed` time
         * since the anchor state, same convention as evaluateIndicators().
         */
        double evaluateOneIndicator(std::size_t idx, double elapsed,
                State const& continuousStates,
                State const& discreteStates,
                Values const& inputs) const;

        /**
         * \brief For event at idx, find the exact zero crossing root between
         * elapsed times a and b (both measured since the same anchor state
         * used by the caller's search window).
         */
        double bisectRoot(std::size_t idx, double a, double b,
            State const& continuousStates,
            State const& discreteStates,
            const Values& inputs) const;

        /**
         * \brief Find events equal to zero for the state as it stands right now
         * (i.e. elapsed = 0, since continuousStates/discreteStates here are the
         * model's actual current, already-integrated state - not a lookahead
         * anchor). This is needed to handle cascading events after a step.
         */
        std::vector<std::size_t> zerosAt(State const& continuousStates,
                            State const& discreteStates,
                            Values const& inputs) const;


        /// @brief  All Events registered to the engine.
        std::vector<Event> events_;

        double tolerance_;

        double maxSearchStepSize_;

        /// @brief The time of the next set of pending events.
        std::optional<double> pendingTime_;

        /// @brief The indices of the pending events to fire at pendingTime_.
        std::vector<std::size_t> pendingEventIndices_;

};

} // frelsim::event
