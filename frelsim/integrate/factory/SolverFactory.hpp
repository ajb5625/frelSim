#pragma once

#include <memory>
#include "../core/Solver.hpp"

namespace frelsim::integrate {

enum class SolverType : int {
    Euler = 0,
    RungeKutta4 = 1,
    DormandPrince = 2,
    BackwardEuler = 4
};


namespace factory {

std::unique_ptr<integrate::core::Solver> createSolver(integrate::SolverType solverType
                                                    , double stopTime
                                                    , double stepSize
                                                    , const Derivative df
                                                    , const JacobianFunction jf = nullptr);

} // factory
} // frelsim::integrate