#pragma once

#include "Component.hpp"

namespace frelsim::model {

class DiscreteComponent : public Component {
    public:
        DiscreteComponent(double sampleTime);

        virtual ~DiscreteComponent() override = default;

        ComponentType type() override;

        double sampleTime() override;

        virtual void update();
    
    private:
        double sampleTime_;

};

} // frelsim::model