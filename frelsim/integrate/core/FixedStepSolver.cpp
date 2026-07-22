#include "FixedStepSolver.hpp"
#include <algorithm>

namespace frelsim::integrate::core {

FixedStepSolver::FixedStepSolver(double stopTime
             , double stepSize
             , const Derivative f
             , const JacobianFunction jf) : Solver(stopTime, f, jf)
                                          , stepSize_(stepSize) {}

FixedStepSolver::~FixedStepSolver() = default;

bool FixedStepSolver::step(State& state, double currentTime, double targetTime) {
    // stepSize_ <= 0 has no meaningful sub-step to take (e.g. a purely
    // discrete component with 0-length continuous state) - just cover the
    // whole interval in one call rather than looping forever.
    if (stepSize_ <= 0.0) {
        singleStep(state, currentTime, targetTime - currentTime);
        return reachedStopTime(targetTime);
    }

    double t = currentTime;
    while (t < targetTime && !util::almostEqual(t, targetTime)) {
        double const dt = std::min(stepSize_, targetTime - t);
        singleStep(state, t, dt);
        t += dt;
    }
    return reachedStopTime(targetTime);
}

} // namespace frelsim::integrate::core
