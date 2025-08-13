#pragma once
#include "../core/Solver.hpp"

namespace frelsim::integrate::expl {


class Euler : public core::Solver {

    public:
        Euler(double stopTime, double stepSize);

        ~Euler() override;

        bool step(State& y0, double simulationTime, const Derivative& f) override;


};


} // namespace frelsim::integrate::explicit