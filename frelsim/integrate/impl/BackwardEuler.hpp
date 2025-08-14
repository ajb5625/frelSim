#pragma once
#include "../core/Solver.hpp"

namespace frelsim::integrate::impl {

class BackwardEuler : public core::Solver {

    public:
        BackwardEuler(double stopTime, 
                    double stepSize, 
                    const Derivative f,
                    const JacobianFunction jf);

        ~BackwardEuler() override = default;

        bool step(State& state, double simulationTime) override;


    private: 
        const int maxIterations_ = 50;


};


}