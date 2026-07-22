#include "DormandPrince45.hpp"

namespace frelsim::integrate::expl {

DormandPrince45::DormandPrince45(double stopTime
                       , double relativeTolerance
                       , double absoluteTolerance
                       , double minStepSize
                       , double maxStepSize
                       , const Derivative f)
    : VariableStepSolver(stopTime, relativeTolerance, absoluteTolerance, minStepSize, maxStepSize, /*order=*/4, f) {}

DormandPrince45::~DormandPrince45() = default;

State DormandPrince45::trialStep(State const& y0, double t0, double h, State& errorEstimate) const {
    // Dormand-Prince RK5(4)7M Butcher tableau. b2 and the 5th-order weight
    // for k7 are both exactly 0 by construction of this tableau (k7 lands
    // on the same point the 5th-order solution does - the "FSAL" property -
    // computed explicitly here for simplicity rather than cached across
    // calls).
    State const k1 = (*f_)(y0, t0);
    State const k2 = (*f_)(y0 + h * (1.0 / 5.0 * k1), t0 + h * (1.0 / 5.0));
    State const k3 = (*f_)(y0 + h * (3.0 / 40.0 * k1 + 9.0 / 40.0 * k2), t0 + h * (3.0 / 10.0));
    State const k4 = (*f_)(y0 + h * (44.0 / 45.0 * k1 - 56.0 / 15.0 * k2 + 32.0 / 9.0 * k3), t0 + h * (4.0 / 5.0));
    State const k5 = (*f_)(y0 + h * (19372.0 / 6561.0 * k1 - 25360.0 / 2187.0 * k2 + 64448.0 / 6561.0 * k3 - 212.0 / 729.0 * k4), t0 + h * (8.0 / 9.0));
    State const k6 = (*f_)(y0 + h * (9017.0 / 3168.0 * k1 - 355.0 / 33.0 * k2 + 46732.0 / 5247.0 * k3 + 49.0 / 176.0 * k4 - 5103.0 / 18656.0 * k5), t0 + h);

    State const y5 = y0 + h * (35.0 / 384.0 * k1 + 500.0 / 1113.0 * k3 + 125.0 / 192.0 * k4 - 2187.0 / 6784.0 * k5 + 11.0 / 84.0 * k6);
    State const k7 = (*f_)(y5, t0 + h);

    // Error estimate = (5th-order weights - 4th-order weights) . k - the
    // difference between the two embedded solutions, without needing to
    // separately assemble the full 4th-order state.
    errorEstimate = h * (
          (35.0 / 384.0 - 5179.0 / 57600.0) * k1
        + (500.0 / 1113.0 - 7571.0 / 16695.0) * k3
        + (125.0 / 192.0 - 393.0 / 640.0) * k4
        + (-2187.0 / 6784.0 - (-92097.0 / 339200.0)) * k5
        + (11.0 / 84.0 - 187.0 / 2100.0) * k6
        + (0.0 - 1.0 / 40.0) * k7
    );

    return y5;
}

} // namespace frelsim::integrate::expl
