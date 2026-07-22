#pragma once
#include "Solver.hpp"

namespace frelsim::integrate::core {

/**
 * \brief A solver that adapts its own step size call to call based on local
 * error estimation against relative/absolute tolerances, rather than owning
 * a single fixed step size. Concrete solvers (e.g. DormandPrince45)
 * implement only trialStep() - the embedded-method-specific computation of
 * a candidate next state plus a local error estimate for a given step size;
 * this base owns the generic accept/reject/step-size-adaptation loop so
 * it's implemented once rather than per method.
 *
 * The error control follows standard practice for embedded Runge-Kutta
 * pairs (the same shape used by e.g. MATLAB's ode45 or SciPy's RK45): a
 * mixed relative+absolute per-component tolerance defines a "scale" for
 * each state component, the local error estimate is normalized by that
 * scale and combined into an RMS norm, and the step size for the next
 * attempt is scaled by that norm raised to -1/(order+1), clamped to sane
 * growth/shrink limits so a single bad step can't make the step size
 * explode or collapse.
 */
class VariableStepSolver : public Solver {

    public:
        VariableStepSolver(double stopTime
             , double relativeTolerance
             , double absoluteTolerance
             , double minStepSize
             , double maxStepSize
             , int order
             , const Derivative f
             , const JacobianFunction jf = nullptr);

        ~VariableStepSolver() override;

        bool step(State& state, double currentTime, double targetTime) override;

    protected:
        /**
         * \brief Compute the candidate next state after one trial step of
         * size dt starting at (state, currentTime), and a per-component
         * local error estimate (same size as state) via errorEstimate.
         * Does not decide whether to accept the step - step() above does
         * that using the returned error estimate.
         */
        virtual State trialStep(State const& state, double currentTime, double dt, State& errorEstimate) const = 0;

        double relativeTolerance_;
        double absoluteTolerance_;
        double minStepSize_;
        double maxStepSize_;

        /// \brief Order of accuracy used for the step-size scaling formula -
        /// e.g. 4 for a 4(5) embedded pair using the lower order for error
        /// control. Set once by the concrete solver's constructor.
        int order_;

    private:
        /// \brief Step size to try first on the next step() call - carried
        /// across calls so an adaptive solver doesn't re-grow from scratch
        /// every time Model::stepUntil calls in with a new target.
        double lastStepSize_;

};

} // namespace frelsim::integrate::core
