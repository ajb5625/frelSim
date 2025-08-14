#pragma once
#include <functional>
#include "../../util/almost_equal.hpp"
#include "../../util/Aliases.hpp"

namespace frelsim::integrate::core {

class Solver {
    
    public:
        Solver(double stopTime
             , double stepSize
             , const Derivative f
             , const JacobianFunction jf = nullptr);

        virtual ~Solver();

        virtual bool step(State& state, double simulationTime) = 0;

    protected:
        double stopTime_;
        double stepSize_;
        Derivative f_;
        JacobianFunction jacobianFunction_;

};


} // namespace frelsim::integrate::core