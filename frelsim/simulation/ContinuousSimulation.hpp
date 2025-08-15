#pragma once
#include <optional>

#include "../util/Aliases.hpp"
#include "Simulation.hpp"

namespace frelsim::simulation {

class ContinuousSimulation : public Simulation {

    public:

        ContinuousSimulation() = default;
        
        virtual ~ContinuousSimulation() override = default;

        SimulationType type() override;

        virtual Derivative derivative() = 0;

        virtual JacobianFunction jacobian() = 0;


};


}