#include "Euler.hpp"


namespace frelsim::integrate::expl {

Euler::Euler(double stopTime, double stepSize, const Derivative& f) : Solver(stopTime, stepSize, f) {}


Euler::~Euler() {}

bool Euler::step(State& y0, double simulationTime) {
    State dydt = (*f_)(y0, simulationTime);
    for (Eigen::Index idx = 0; idx < y0.size(); idx++) {
        y0[idx] = y0[idx] + stepSize_ * dydt[idx];
    }
    return util::almostEqual(simulationTime, stopTime_);
}

} // namespace frelsim::integrate::expl