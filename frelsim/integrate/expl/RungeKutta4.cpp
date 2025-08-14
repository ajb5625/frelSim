#include "RungeKutta4.hpp"

namespace frelsim::integrate::expl {

RungeKutta4::RungeKutta4(double stopTime
                        , double stepSize
                        , const Derivative f) : Solver(stopTime, stepSize, f), halfStep_(stepSize / 2) {}

bool RungeKutta4::step(State& yn, double simulationTime) {
    State k1 = (*f_)(yn, simulationTime); 
    State k2 = (*f_)(yn + (halfStep_) * k1, simulationTime + halfStep_);
    State k3 = (*f_)(yn + (halfStep_) * k2, simulationTime + halfStep_);
    State k4 = (*f_)(yn + stepSize_ * k3, simulationTime + stepSize_);

    yn += (stepSize_ / 6) * (k1 + 2 * k2 + 2 * k3 + k4);
    return util::almostEqual(simulationTime, stopTime_);
}



} // frelsim::integrate::expl