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
         * \brief At time, evaluate all event indicators.
         */
        std::vector<double> evaluateIndicators(double time
                                            , State const& continuousStates
                                            , State const& discreteStates
                                            , Values const& inputs) const;

        /**
         * \brief At time, evaluate the indicator corresponding to idx.
         */
        double evaluateOneIndicator(std::size_t idx, double time,
                State const& continuousStates,
                State const& discreteStates,
                Values const& inputs) const;

        /**
         * \brief For event at idx, find the exact zero crossing root
         * between a and b.
         */
        double bisectRoot(std::size_t idx, double a, double b,
            State const& continuousStates,
            State const& discreteStates,
            const Values& inputs) const;

        /**
         * \brief Find events equal to zero at time.
         * This is needed to handle cascading events.
         */
        std::vector<std::size_t> zerosAt(double time,
                            State const& continuousStates,
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
