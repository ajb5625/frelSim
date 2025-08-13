#pragma once
#include <functional>

#include "../../util/Aliases.hpp"

namespace frelsim::integrate::core {

class Solver {
    
    public:
        Solver(double stopTime, double stepSize);

        virtual ~Solver();

        virtual bool step(State& state, double simulationTime, const Derivative& f) = 0;

    protected:
        double stopTime_;
        double stepSize_;

};


} // namespace frelsim::integrate::core