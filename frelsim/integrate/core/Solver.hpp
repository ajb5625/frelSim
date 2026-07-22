#pragma once
#include <functional>
#include "../../util/almost_equal.hpp"
#include "../../util/Aliases.hpp"

namespace frelsim::integrate::core {

/**
 * \brief The truly common contract across every numerical ODE solver:
 * something to integrate (f_, and optionally its Jacobian for implicit
 * methods), an overall stopTime to know when the whole simulation is done,
 * and a way to advance state from one time to another. HOW that advance
 * happens differs fundamentally between fixed-step solvers (own a step
 * size, sub-step internally - see FixedStepSolver.hpp) and variable-step
 * solvers (adapt their own step size call to call based on local error
 * control - see VariableStepSolver.hpp), so this base deliberately doesn't
 * assume either shape.
 */
class Solver {

    public:
        Solver(double stopTime
             , const Derivative f
             , const JacobianFunction jf = nullptr);

        virtual ~Solver();

        /**
         * \brief Advance state from currentTime to targetTime.
         * \returns true once targetTime reaches this solver's overall stopTime.
         */
        virtual bool step(State& state, double currentTime, double targetTime) = 0;

    protected:
        bool reachedStopTime(double targetTime) const;

        double stopTime_;
        Derivative f_;
        JacobianFunction jacobianFunction_;

};


} // namespace frelsim::integrate::core
