#pragma once
#include "Solver.hpp"

namespace frelsim::integrate::core {

/**
 * \brief A solver that owns a fixed step size for accuracy/stability and
 * sub-steps internally in increments of at most that size to cover whatever
 * interval step() is asked to cover - the usual fixed-step-solver contract.
 * Concrete solvers (Euler, RungeKutta4, BackwardEuler) implement only
 * singleStep(); this base owns the sub-stepping loop so it isn't
 * duplicated across them.
 */
class FixedStepSolver : public Solver {

    public:
        FixedStepSolver(double stopTime
             , double stepSize
             , const Derivative f
             , const JacobianFunction jf = nullptr);

        ~FixedStepSolver() override;

        /**
         * \brief Advance state from currentTime to targetTime, taking as
         * many sub-steps of at most stepSize_ as needed to get there (the
         * caller - Model::stepUntil, ultimately driven by Simulator - can
         * be forced to request a smaller-than-usual interval to land
         * exactly on a discrete task or event boundary, and a larger one is
         * simply covered by more sub-steps).
         */
        bool step(State& state, double currentTime, double targetTime) override;

    protected:
        /**
         * \brief One actual integration step of size dt, starting at
         * currentTime. Implemented by each concrete solver; step() above
         * calls this as many times as needed to cover the requested
         * interval.
         */
        virtual void singleStep(State& state, double currentTime, double dt) = 0;

        double stepSize_;

};


} // namespace frelsim::integrate::core
