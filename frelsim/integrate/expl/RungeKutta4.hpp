#pragma once
#include "../core/Solver.hpp"

namespace frelsim::integrate::expl {

class RungeKutta4 final : public core::Solver {
    public:
        RungeKutta4(double stopTime
                    , double stepSize
                    , const Derivative f);

        ~RungeKutta4() override;

    protected:
        void singleStep(State& state, double currentTime, double dt) override;
};
}
