#include "SolverFactory.hpp"
#include <map>
#include "../analysis/StiffnessDetector.hpp"

namespace frelsim::integrate::factory {

namespace {

// Function-local static: guarantees the registry exists before the first
// registerSolver() call regardless of static-initialization order across
// translation units (the "static initialization order fiasco").
std::map<sim::proto::SolverType, SolverCreator>& registry() {
    static std::map<sim::proto::SolverType, SolverCreator> instance;
    return instance;
}

// Resolves Automatic to a concrete SolverType via a one-time stiffness
// diagnostic; passes every other SolverType through unchanged.
sim::proto::SolverType resolveSolverType(sim::proto::SolverType solverType
                                        , const JacobianFunction jf
                                        , State const& initialState) {
    if (solverType != sim::proto::SolverType::Automatic) {
        return solverType;
    }
    if (jf == nullptr || initialState.size() == 0) {
        return sim::proto::SolverType::DormandPrince;
    }
    Matrix const jacobian = (*jf)(initialState, /*t=*/0.0);
    return analysis::assessStiffness(jacobian).isStiff
        ? sim::proto::SolverType::BackwardEuler
        : sim::proto::SolverType::DormandPrince;
}

} // namespace

bool registerSolver(sim::proto::SolverType solverType, SolverCreator creator) {
    registry()[solverType] = std::move(creator);
    return true;
}

std::unique_ptr<integrate::core::Solver> createSolver(frelsim::sim::proto::SolverType solverType
                                                                    , double stopTime
                                                                    , SolverConfig const& config
                                                                    , const Derivative f
                                                                    , const JacobianFunction jf
                                                                    , State const& initialState) {
    sim::proto::SolverType const resolvedType = resolveSolverType(solverType, jf, initialState);
    auto it = registry().find(resolvedType);
    if (it == registry().end()) {
        return nullptr;
    }
    return it->second(stopTime, config, f, jf);
}

} // frelsim::integrate::factory
