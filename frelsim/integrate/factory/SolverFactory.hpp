#pragma once

#include <memory>
#include "../core/Solver.hpp"
#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::integrate {

// enum class SolverType : int {
    // Euler = 0,
    // RungeKutta4 = 1,
    // DormandPrince = 2,
    // BackwardEuler = 4
// };


namespace factory {

/**
 * \brief Tuning knobs for solver construction. Not every field is relevant
 * to every solver family: stepSize is used by FixedStepSolver-derived
 * solvers (Euler/RungeKutta4/BackwardEuler); relativeTolerance/
 * absoluteTolerance/minStepSize/maxStepSize are used by
 * VariableStepSolver-derived solvers (DormandPrince45). createSolver below
 * picks whichever subset the requested SolverType actually needs.
 */
struct SolverConfig {
    double stepSize = 0.0;

    double relativeTolerance = 1e-6;
    double absoluteTolerance = 1e-9;
    double minStepSize = 1e-10;
    // 0 is not a usable value for a variable-step solver - callers must set
    // this from the component's own configured max_step_size/period before
    // requesting a variable-step solver type.
    double maxStepSize = 0.0;
};

std::unique_ptr<integrate::core::Solver> createSolver(frelsim::sim::proto::SolverType solverType
                                                    , double stopTime
                                                    , SolverConfig const& config
                                                    , const Derivative df
                                                    , const JacobianFunction jf = nullptr);

} // factory
} // frelsim::integrate
