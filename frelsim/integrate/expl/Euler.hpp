#pragma once
#include "../core/Solver.hpp"

namespace frelsim::integrate::expl {


class Euler : public core::Solver {

    public:
        Euler(double stopTime, double stepSize, const Derivative& f);

        ~Euler() override;

        bool step(State& y0, double simulationTime) override;


};


} // namespace frelsim::integrate::explicit