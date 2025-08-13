#include "BackwardEuler.hpp"


namespace frelsim::integrate::impl {

BackwardEuler::BackwardEuler(double stopTime, double stepSize) : Solver(stopTime, stepSize) {}

bool BackwardEuler::step(State& state, double simulationTime, const Derivative& f) {
    return false;
}


}