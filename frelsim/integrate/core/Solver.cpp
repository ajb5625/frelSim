#include "Solver.hpp"

namespace frelsim::integrate::core {

Solver::Solver(double stopTime
             , const Derivative f
             , const JacobianFunction jf) : stopTime_(stopTime)
                                          , f_(f)
                                          , jacobianFunction_(jf) {}

Solver::~Solver() = default;

bool Solver::reachedStopTime(double targetTime) const {
    return util::almostEqual(targetTime, stopTime_);
}

} // namespace frelsim::integrate::core
