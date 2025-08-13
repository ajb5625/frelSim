#pragma once
#include <optional>

#include "../util/Aliases.hpp"
#include "Component.hpp"

namespace frelsim::model {

class ContinuousComponent : public Component {

    public:

        ContinuousComponent() = default;
        
        virtual ~ContinuousComponent() override = default;

        ComponentType type() override;

        double sampleTime() override;

        virtual Derivative derivative() = 0;

        virtual std::optional<Matrix> jacobian();


};


}