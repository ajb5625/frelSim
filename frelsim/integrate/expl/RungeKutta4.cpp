#include "RungeKutta4.hpp"

namespace frelsim::integrate::expl {

RungeKutta4::RungeKutta4(double stopTime
                        , double stepSize
                        , const Derivative f) : Solver(stopTime, stepSize, f) {}

RungeKutta4::~RungeKutta4() = default;

void RungeKutta4::singleStep(State& yn, double currentTime, double dt) {
    double const halfStep = dt / 2.0;
    State k1 = (*f_)(yn, currentTime);
    State k2 = (*f_)(yn + halfStep * k1, currentTime + halfStep);
    State k3 = (*f_)(yn + halfStep * k2, currentTime + halfStep);
    State k4 = (*f_)(yn + dt * k3, currentTime + dt);

    yn += (dt / 6.0) * (k1 + 2 * k2 + 2 * k3 + k4);
}



} // frelsim::integrate::expl
