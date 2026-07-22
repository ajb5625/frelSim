#include "SolverFactory.hpp"
#include "../expl/Euler.hpp"
#include "../expl/RungeKutta4.hpp"
#include "../expl/DormandPrince45.hpp"
#include "../impl/BackwardEuler.hpp"

namespace frelsim::integrate::factory{

std::unique_ptr<integrate::core::Solver> createSolver(frelsim::sim::proto::SolverType solverType
                                                                    , double stopTime
                                                                    , SolverConfig const& config
                                                                    , const Derivative f
                                                                    , const JacobianFunction jf) {

    switch(solverType) {
        case sim::proto::SolverType::Euler:
            return std::make_unique<integrate::expl::Euler>(stopTime, config.stepSize, f);
        break;
        case sim::proto::SolverType::RungeKutta4:
            return std::make_unique<integrate::expl::RungeKutta4>(stopTime, config.stepSize, f);
        break;
        case sim::proto::SolverType::DormandPrince:
            return std::make_unique<integrate::expl::DormandPrince45>(stopTime
                                                                     , config.relativeTolerance
                                                                     , config.absoluteTolerance
                                                                     , config.minStepSize
                                                                     , config.maxStepSize
                                                                     , f);
        break;
        case sim::proto::SolverType::BackwardEuler:
            return std::make_unique<integrate::impl::BackwardEuler>(stopTime, config.stepSize, f, jf);
        break;
        default:
            return nullptr;
        break;
    }
}
} // frelsim::integrate::factory

