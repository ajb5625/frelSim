#pragma once

#include "../../util/Aliases.hpp"

namespace frelsim::integrate::analysis {

struct StiffnessAssessment {
    bool isStiff;
    // max|Re(lambda)| / min|Re(lambda)| over the Jacobian's stable
    // (negative-real-part) eigenvalues. Kept alongside isStiff for
    // diagnostics/logging rather than only exposing the boolean verdict.
    double stiffnessRatio;
};

/**
 * \brief Classifies stiffness from a single Jacobian sample using the
 * classic ratio-of-eigenvalue-real-parts heuristic: a large disparity
 * between a system's fastest- and slowest-decaying stable modes forces an
 * explicit solver's step size down to chase the fast mode's stability limit
 * long after that mode has actually decayed away, whereas an implicit
 * method's stability region isn't bounded that way. This is a real but
 * inherently approximate diagnostic (a single sample, not a stiffness
 * proof) - good enough to pick a solver family once at initialization, not
 * a substitute for the model author's own judgment.
 *
 * Eigenvalues with a non-negative real part (marginal or unstable modes)
 * are ignored when forming the ratio, since they don't impose the explicit
 * step-size restriction stiffness is actually about. A Jacobian with no
 * stable modes at all (or no continuous states) is reported as not stiff.
 *
 * \param threshold Stiffness ratio at or above which the system is reported
 * stiff. 1000 is a commonly cited rule-of-thumb cutoff; callers with
 * domain-specific knowledge may want a different value.
 */
StiffnessAssessment assessStiffness(Matrix const& jacobian, double threshold = 1000.0);

} // namespace frelsim::integrate::analysis
