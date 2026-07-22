#pragma once
#include "../core/VariableStepSolver.hpp"

namespace frelsim::integrate::expl {

/**
 * \brief Dormand-Prince embedded 5(4) Runge-Kutta method (the same method
 * behind MATLAB's ode45 and SciPy's RK45): a variable-step explicit solver
 * that estimates its own local error each step by comparing a 4th- and
 * 5th-order solution computed from the same 7 stage evaluations, and feeds
 * that estimate into VariableStepSolver's step-size adaptation.
 */
class DormandPrince45 final : public core::VariableStepSolver {
    public:
        DormandPrince45(double stopTime
                       , double relativeTolerance
                       , double absoluteTolerance
                       , double minStepSize
                       , double maxStepSize
                       , const Derivative f);

        ~DormandPrince45() override;

    protected:
        State trialStep(State const& state, double currentTime, double dt, State& errorEstimate) const override;
};

} // namespace frelsim::integrate::expl
