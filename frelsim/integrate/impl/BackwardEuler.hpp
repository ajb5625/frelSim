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

    protected:
        void singleStep(State& state, double currentTime, double dt) override;


    private:
        const int maxIterations_ = 50;


};


}
