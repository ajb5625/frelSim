#pragma once
#include "../core/FixedStepSolver.hpp"

namespace frelsim::integrate::expl {


class Euler : public core::FixedStepSolver {

    public:
        Euler(double stopTime, double stepSize, const Derivative& f);

        ~Euler() override;

    protected:
        void singleStep(State& y0, double currentTime, double dt) override;


};


} // namespace frelsim::integrate::explicit
