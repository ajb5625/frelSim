#include "Euler.hpp"


namespace frelsim::integrate::expl {

Euler::Euler(double stopTime, double stepSize) : Solver(stopTime, stepSize) {}


Euler::~Euler() {}

bool Euler::step(State& y0, double simulationTime, const Derivative& f) {
    State dydt = (*f)(y0, simulationTime);
    for (std::size_t idx = 0; idx < y0.size(); idx++) {
        y0[idx] = y0[idx] + stepSize_ * dydt[idx];
    }
    return simulationTime == stopTime_;
}

} // namespace frelsim::integrate::expl