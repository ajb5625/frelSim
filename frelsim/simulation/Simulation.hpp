#pragma once
#include <functional>
#include <vector>
#include <map>
#include <memory>

#include "../util/Aliases.hpp"
#include "../event/Event.hpp"

namespace frelsim::simulation {

enum class SimulationType : int {
    Continuous = 0,
    Discrete = 1
};

using Parameter = double;

class Simulation {

    public:

        Simulation() = default;

        virtual ~Simulation();

        virtual void stepUntil(double stopTime) = 0;

        virtual SimulationType type() = 0;

        virtual void parameters();

        virtual void events();

        std::vector<event::EventIndicator> eventIndicators() const;

        virtual void sampleTime();

    private:

        State states_;

        std::vector<event::Event> events_;    

        std::map<std::string, Parameter> parameters_;



};


} // frelsim::simulation 