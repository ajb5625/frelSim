#pragma once
#include <functional>
#include <vector>
#include <map>
#include <memory>

#include "../util/Aliases.hpp"
#include "../event/Event.hpp"

namespace frelsim::model {

enum class ComponentType : int {
    Continuous = 0,
    Discrete = 1
};

using Parameter = double;

class Component {

    public:

        Component() = default;

        virtual ~Component();

        virtual ComponentType type() = 0;

        virtual double sampleTime() = 0;

        virtual void parameters();

        virtual void events();

        std::vector<event::EventIndicator> eventIndicators() const;

    private:

        State states_;

        std::vector<event::Event> events_;    

        std::map<std::string, Parameter> parameters_;




};


} // frelsim::model 