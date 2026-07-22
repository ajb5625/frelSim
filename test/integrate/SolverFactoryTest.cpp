#include <gtest/gtest.h>
#include <cmath>
#include "frelsim/integrate/factory/SolverFactory.hpp"

namespace frelsim::integrate::factory {
namespace {

// Every solver self-registers from its own .cpp via FRELSIM_REGISTER_SOLVER
// (see SolverFactory.hpp) rather than SolverFactory.cpp switching over a
// hardcoded list of concrete types. That means a broken registration (e.g.
// a solver's .o silently dropped from the static archive link, or a typo in
// the SolverType a solver registers under) would show up as createSolver
// quietly returning nullptr instead of a compile error - exactly what this
// test suite exists to catch.

Derivative exponentialDecayDerivative() {
    return std::make_shared<std::function<State(State const&, double)>>(
        [](State const& y, double) -> State {
            State dydt(1);
            dydt[0] = -y[0];
            return dydt;
        });
}

JacobianFunction exponentialDecayJacobian() {
    return std::make_shared<std::function<Matrix(State const&, double)>>(
        [](State const&, double) -> Matrix {
            Matrix jacobian(1, 1);
            jacobian(0, 0) = -1.0;
            return jacobian;
        });
}

TEST(SolverFactoryTest, CreatesEachRegisteredSolverTypeAndIntegratesCorrectly) {
    SolverConfig config;
    config.stepSize = 0.001;
    config.maxStepSize = 0.1;

    for (sim::proto::SolverType type : {sim::proto::SolverType::Euler
                                       , sim::proto::SolverType::RungeKutta4
                                       , sim::proto::SolverType::DormandPrince
                                       , sim::proto::SolverType::BackwardEuler}) {
        auto solver = createSolver(type, /*stopTime=*/1.0, config
                                  , exponentialDecayDerivative(), exponentialDecayJacobian());

        ASSERT_NE(solver, nullptr) << "SolverType " << sim::proto::SolverType_Name(type)
                                    << " did not register with the factory";

        State y(1);
        y[0] = 1.0;
        solver->step(y, 0.0, 1.0);

        EXPECT_NEAR(y[0], std::exp(-1.0), 1e-2) << "SolverType " << sim::proto::SolverType_Name(type);
    }
}

TEST(SolverFactoryTest, ReturnsNullptrForAnUnregisteredSolverType) {
    SolverConfig config;
    auto solver = createSolver(static_cast<sim::proto::SolverType>(-1), /*stopTime=*/1.0, config
                              , exponentialDecayDerivative());

    EXPECT_EQ(solver, nullptr);
}

} // namespace
} // namespace frelsim::integrate::factory
