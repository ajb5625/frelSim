#include "SolverFactory.hpp"
#include <map>

namespace frelsim::integrate::factory {

namespace {

// Function-local static: guarantees the registry exists before the first
// registerSolver() call regardless of static-initialization order across
// translation units (the "static initialization order fiasco").
std::map<sim::proto::SolverType, SolverCreator>& registry() {
    static std::map<sim::proto::SolverType, SolverCreator> instance;
    return instance;
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
                                                                    , const JacobianFunction jf) {
    auto it = registry().find(solverType);
    if (it == registry().end()) {
        return nullptr;
    }
    return it->second(stopTime, config, f, jf);
}

} // frelsim::integrate::factory
