#pragma once
#include <optional>
#include <vector>
#include "Event.hpp"

namespace frelsim::event {

class EventEngine final {

    public:
        EventEngine(std::vector<Event>const& events,
                    double tolerance,
                    double maxSearchStep);

        ~EventEngine() = default;

        double nextEventTime(double t0
                            , double maxTime
                            , State const& continuousStates
                            , State const& discreteStates
                            , Values const& inputs);

        bool processEventsAt(double currentTime
                            , State& continuousStates
                            , State& discreteStates
                            , Parameters& params
                            , Values const& inputs
                            , Values& outputs);


    private:

        void clearPending();

        std::vector<double> evalAll(double time
                                    , State const& continuousStates
                                    , State const& discreteStates
                                    , Values const& inputs) const;

        double evalOne(std::size_t idx, double time,
                State const& continuousStates,
                State const& discreteStates,
                Values const& inputs) const;

        double bisectRoot(std::size_t idx, double a, double b,
            State const& continuousStates,
            State const& discreteStates,
            const Values& inputs) const;

        std::vector<std::size_t> zerosAt(double time,
                            State const& continuousStates,
                            State const& discreteStates,
                            Values const& inputs) const;


        std::vector<Event> events_;

        double tolerance_;

        double maxSearchStepSize_;

        std::optional<double> pendingTime_;

        std::vector<std::size_t> pendingEventIndices_;

};

} // frelsim::event
