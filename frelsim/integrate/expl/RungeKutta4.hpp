#pragma once
#include "../core/Solver.hpp"

namespace frelsim::integrate::expl {

class RungeKutta4 final : public core::Solver {
    public:
        RungeKutta4(double stopTime
                    , double stepSize
                    , const Derivative f);

        ~RungeKutta4() override;

        bool step(State& state, double simulationTime) override;
    private: 
        double halfStep_;

};
}