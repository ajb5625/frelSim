#include "Euler.hpp"


namespace frelsim::integrate::expl {

Euler::Euler(double stopTime, double stepSize, const Derivative& f) : Solver(stopTime, stepSize, f) {}


Euler::~Euler() {}

void Euler::singleStep(State& y0, double currentTime, double dt) {
    State dydt = (*f_)(y0, currentTime);
    for (Eigen::Index idx = 0; idx < y0.size(); idx++) {
        y0[idx] = y0[idx] + dt * dydt[idx];
    }
}

} // namespace frelsim::integrate::expl
