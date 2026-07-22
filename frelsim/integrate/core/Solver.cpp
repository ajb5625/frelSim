#include "Solver.hpp"

namespace frelsim::integrate::core {

Solver::Solver(double stopTime
             , double stepSize
             , const Derivative f
             , const JacobianFunction jf) : stopTime_(stopTime)
                                          , stepSize_(stepSize)
                                          , f_(f)
                                          , jacobianFunction_(jf) {}

Solver::~Solver() = default;

bool Solver::step(State& state, double currentTime, double targetTime) {
    // stepSize_ <= 0 has no meaningful sub-step to take (e.g. a purely
    // discrete component with 0-length continuous state) - just cover the
    // whole interval in one call rather than looping forever.
    if (stepSize_ <= 0.0) {
        singleStep(state, currentTime, targetTime - currentTime);
        return util::almostEqual(targetTime, stopTime_);
    }

    double t = currentTime;
    while (t < targetTime && !util::almostEqual(t, targetTime)) {
        double const dt = std::min(stepSize_, targetTime - t);
        singleStep(state, t, dt);
        t += dt;
    }
    return util::almostEqual(targetTime, stopTime_);
}

} // namespace frelsim::integrate::core
