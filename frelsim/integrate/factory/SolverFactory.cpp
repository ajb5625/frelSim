#include "SolverFactory.hpp"
#include "../expl/Euler.hpp"
#include "../expl/RungeKutta4.hpp"
#include "../impl/BackwardEuler.hpp"

namespace frelsim::integrate::factory{

std::unique_ptr<integrate::core::Solver> createSolver(integrate::SolverType solverType
                                                                    , double stopTime
                                                                    , double stepSize
                                                                    , const Derivative f
                                                                    , const JacobianFunction jf) {

    switch(solverType) {
        case integrate::SolverType::Euler:
            return std::make_unique<integrate::expl::Euler>(stopTime, stepSize, f);
        break;
        case integrate::SolverType::RungeKutta4:
            return std::make_unique<integrate::expl::RungeKutta4>(stopTime, stepSize, f);
        break;
        case integrate::SolverType::DormandPrince:
            return nullptr;
        break;
        case integrate::SolverType::BackwardEuler:
            return std::make_unique<integrate::impl::BackwardEuler>(stopTime, stepSize, f, jf);
        break;
        default:
            return nullptr;
        break;
    }   
}
} // frelsim::integrate::factory

