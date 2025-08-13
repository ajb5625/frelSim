#pragma once
#include "../core/Solver.hpp"

namespace frelsim::integrate::impl {

class BackwardEuler : public core::Solver {

    public:
        BackwardEuler(double stopTime, double stepSize);

        ~BackwardEuler() override = default;

        bool step(State& state, double simulationTime, const Derivative& f) override;


};


}