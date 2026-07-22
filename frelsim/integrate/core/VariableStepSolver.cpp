#include "VariableStepSolver.hpp"
#include <algorithm>
#include <cmath>

namespace frelsim::integrate::core {

namespace {

constexpr double kSafetyFactor = 0.9;
constexpr double kMaxGrowth = 5.0;
constexpr double kMaxShrink = 0.1;
// A run of rejections this long at the smallest allowed step size means the
// tolerance genuinely can't be met at minStepSize_ - a configuration
// problem, not something to hang the simulation on.
constexpr int kMaxRejectionsPerStep = 100;

} // namespace

VariableStepSolver::VariableStepSolver(double stopTime
             , double relativeTolerance
             , double absoluteTolerance
             , double minStepSize
             , double maxStepSize
             , int order
             , const Derivative f
             , const JacobianFunction jf) : Solver(stopTime, f, jf)
                                          , relativeTolerance_(relativeTolerance)
                                          , absoluteTolerance_(absoluteTolerance)
                                          , minStepSize_(minStepSize)
                                          , maxStepSize_(maxStepSize)
                                          , order_(order)
                                          , lastStepSize_(maxStepSize) {}

VariableStepSolver::~VariableStepSolver() = default;

bool VariableStepSolver::step(State& state, double currentTime, double targetTime) {
    double t = currentTime;
    double h = lastStepSize_;

    while (t < targetTime && !util::almostEqual(t, targetTime)) {
        h = std::clamp(h, minStepSize_, maxStepSize_);
        h = std::min(h, targetTime - t); // never overshoot the target

        int rejections = 0;
        while (true) {
            State errorEstimate;
            State proposed = trialStep(state, t, h, errorEstimate);

            // Mixed relative+absolute scaled RMS error norm - standard
            // practice for embedded RK pairs. errorNorm <= 1.0 means the
            // step met tolerance. Eigen's array expressions do this
            // elementwise (and vectorized) instead of a manual index loop.
            Eigen::ArrayXd const scale = absoluteTolerance_
                + relativeTolerance_ * state.array().abs().max(proposed.array().abs());
            Eigen::ArrayXd const ratio = errorEstimate.array() / scale;
            double const errorNorm = state.size() > 0
                ? std::sqrt(ratio.square().sum() / static_cast<double>(state.size()))
                : 0.0;

            // Same formula scales the step down after a rejection and up
            // after an acceptance - it naturally does the right thing in
            // both directions depending on whether errorNorm is above or
            // below 1.
            double const growth = (errorNorm > 0.0)
                ? std::clamp(kSafetyFactor * std::pow(errorNorm, -1.0 / (order_ + 1)), kMaxShrink, kMaxGrowth)
                : kMaxGrowth;

            bool const atFloor = h <= minStepSize_ * (1.0 + 1e-12);
            if (errorNorm <= 1.0 || atFloor || rejections >= kMaxRejectionsPerStep) {
                state = proposed;
                t += h;
                h = std::clamp(h * growth, minStepSize_, maxStepSize_);
                break;
            }

            h = std::clamp(h * growth, minStepSize_, maxStepSize_);
            ++rejections;
        }
    }

    lastStepSize_ = h;
    return reachedStopTime(targetTime);
}

} // namespace frelsim::integrate::core
