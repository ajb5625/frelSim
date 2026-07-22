#pragma once

#include <functional>
#include <memory>
#include "../core/Solver.hpp"
#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::integrate::factory {

/**
 * \brief Tuning knobs for solver construction. Not every field is relevant
 * to every solver family: stepSize is used by FixedStepSolver-derived
 * solvers (Euler/RungeKutta4/BackwardEuler); relativeTolerance/
 * absoluteTolerance/minStepSize/maxStepSize are used by
 * VariableStepSolver-derived solvers (DormandPrince45). Each solver's own
 * registration (see FRELSIM_REGISTER_SOLVER below) picks whichever subset it
 * actually needs.
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

using SolverCreator = std::function<std::unique_ptr<core::Solver>(
    double stopTime, SolverConfig const&, Derivative const&, JacobianFunction const&)>;

/**
 * \brief Registers `creator` under `solverType`, so a later createSolver()
 * call with that SolverType dispatches to it. Not normally called directly -
 * see FRELSIM_REGISTER_SOLVER below, which each Solver subclass uses to
 * register itself from its own .cpp, so SolverFactory never needs to
 * #include (or otherwise know about) any concrete Solver.
 * \returns true, so it can be used as a static initializer's value.
 */
bool registerSolver(sim::proto::SolverType solverType, SolverCreator creator);

/**
 * \brief Constructs the Solver registered under solverType, or nullptr if no
 * solver was registered under it.
 */
std::unique_ptr<core::Solver> createSolver(frelsim::sim::proto::SolverType solverType
                                                    , double stopTime
                                                    , SolverConfig const& config
                                                    , const Derivative df
                                                    , const JacobianFunction jf = nullptr);

} // frelsim::integrate::factory

/**
 * \brief Place at namespace scope in a Solver subclass's .cpp to register it
 * under `solverType`, e.g.:
 *   FRELSIM_REGISTER_SOLVER(::frelsim::sim::proto::SolverType::Euler,
 *       [](double stopTime, ::frelsim::integrate::factory::SolverConfig const& config,
 *          ::frelsim::Derivative const& f, ::frelsim::JacobianFunction const&) {
 *           return std::make_unique<Euler>(stopTime, config.stepSize, f);
 *       })
 *
 * The creator is a full lambda rather than something generated from a single
 * generic shape, because solver constructors genuinely differ by family -
 * fixed-step vs. variable-step, and jacobian-using vs. not - see Euler.cpp,
 * DormandPrince45.cpp, and BackwardEuler.cpp for the three different shapes
 * actually in use. The macro only removes the registry-plumbing boilerplate,
 * not the part that's legitimately different per solver.
 *
 * This relies on the registration running as a static initializer when the
 * subclass's .o is linked in. Since libfrelsim is a static archive (.a) and
 * nothing else references these .o files once SolverFactory itself doesn't
 * #include them, the linker would otherwise drop them entirely - the
 * Makefile links libfrelsim with --whole-archive specifically so this works;
 * see the LINK_LIBFRELSIM comment there before changing how libfrelsim.a is
 * linked into an executable. (Same mechanism FRELSIM_REGISTER_MODEL uses -
 * see ModelFactory.hpp.)
 */
#define FRELSIM_REGISTER_SOLVER(solverType, creator) \
    namespace { \
        const bool registered_ = ::frelsim::integrate::factory::registerSolver(solverType, creator); \
    }
