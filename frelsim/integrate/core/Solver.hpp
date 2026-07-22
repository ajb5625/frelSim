#pragma once
#include <algorithm>
#include <functional>
#include "../../util/almost_equal.hpp"
#include "../../util/Aliases.hpp"

namespace frelsim::integrate::core {

class Solver {

    public:
        Solver(double stopTime
             , double stepSize
             , const Derivative f
             , const JacobianFunction jf = nullptr);

        virtual ~Solver();

        /**
         * \brief Advance state from currentTime to targetTime, taking as
         * many sub-steps of at most stepSize_ as needed to get there - the
         * usual fixed-step-solver contract: this solver owns its own step
         * size for accuracy/stability and sub-steps internally to cover
         * whatever interval it's asked to cover, rather than trusting the
         * caller to always ask in exact multiples of that step size (the
         * caller - Model::stepUntil, ultimately driven by Simulator - can
         * be forced to request a smaller-than-usual interval to land
         * exactly on a discrete task or event boundary, and a larger one is
         * simply covered by more sub-steps).
         * \returns true once targetTime reaches this solver's overall stopTime.
         */
        bool step(State& state, double currentTime, double targetTime);

    protected:
        /**
         * \brief One actual integration step of size dt, starting at
         * currentTime. Implemented by each concrete solver; step() above
         * calls this as many times as needed to cover the requested
         * interval.
         */
        virtual void singleStep(State& state, double currentTime, double dt) = 0;

        double stopTime_;
        double stepSize_;
        Derivative f_;
        JacobianFunction jacobianFunction_;

};


} // namespace frelsim::integrate::core
