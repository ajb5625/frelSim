#include <gtest/gtest.h>
#include <cmath>
#include "frelsim/integrate/factory/SolverFactory.hpp"
#include "frelsim/integrate/expl/DormandPrince45.hpp"
#include "frelsim/integrate/impl/BackwardEuler.hpp"

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

// dy/dt = -1000*y is stiff on its own (a single mode has nothing slower to
// be compared against, per StiffnessDetectorTest), so pair it with a second,
// much slower state to actually produce the widely-separated decay rates
// stiffness is about.
JacobianFunction stiffJacobian() {
    return std::make_shared<std::function<Matrix(State const&, double)>>(
        [](State const&, double) -> Matrix {
            Matrix jacobian(2, 2);
            jacobian << -1000.0, 0.0,
                            0.0, -1.0;
            return jacobian;
        });
}

JacobianFunction nonStiffJacobian() {
    return std::make_shared<std::function<Matrix(State const&, double)>>(
        [](State const&, double) -> Matrix {
            Matrix jacobian(2, 2);
            jacobian << -1.0, 0.0,
                         0.0, -2.0;
            return jacobian;
        });
}

Derivative twoStateDecayDerivative() {
    return std::make_shared<std::function<State(State const&, double)>>(
        [](State const& y, double) -> State {
            State dydt(2);
            dydt[0] = -1000.0 * y[0];
            dydt[1] = -1.0 * y[1];
            return dydt;
        });
}

TEST(SolverFactoryTest, AutomaticPicksBackwardEulerWhenTheJacobianIsStiff) {
    SolverConfig config;
    config.stepSize = 0.001;
    config.maxStepSize = 0.1;
    State initialState(2);
    initialState << 1.0, 1.0;

    auto solver = createSolver(sim::proto::SolverType::Automatic, /*stopTime=*/1.0, config
                              , twoStateDecayDerivative(), stiffJacobian(), initialState);

    ASSERT_NE(solver, nullptr);
    EXPECT_NE(dynamic_cast<impl::BackwardEuler*>(solver.get()), nullptr);
}

TEST(SolverFactoryTest, AutomaticPicksDormandPrinceWhenTheJacobianIsNotStiff) {
    SolverConfig config;
    config.maxStepSize = 0.1;
    State initialState(2);
    initialState << 1.0, 1.0;

    auto solver = createSolver(sim::proto::SolverType::Automatic, /*stopTime=*/1.0, config
                              , twoStateDecayDerivative(), nonStiffJacobian(), initialState);

    ASSERT_NE(solver, nullptr);
    EXPECT_NE(dynamic_cast<expl::DormandPrince45*>(solver.get()), nullptr);
}

TEST(SolverFactoryTest, AutomaticFallsBackToDormandPrinceWithoutAJacobian) {
    SolverConfig config;
    config.maxStepSize = 0.1;
    State initialState(1);
    initialState << 1.0;

    auto solver = createSolver(sim::proto::SolverType::Automatic, /*stopTime=*/1.0, config
                              , exponentialDecayDerivative(), /*jf=*/nullptr, initialState);

    ASSERT_NE(solver, nullptr);
    EXPECT_NE(dynamic_cast<expl::DormandPrince45*>(solver.get()), nullptr);
}

TEST(SolverFactoryTest, AutomaticFallsBackToDormandPrinceWithoutAnInitialState) {
    SolverConfig config;
    config.maxStepSize = 0.1;

    auto solver = createSolver(sim::proto::SolverType::Automatic, /*stopTime=*/1.0, config
                              , exponentialDecayDerivative(), exponentialDecayJacobian());

    ASSERT_NE(solver, nullptr);
    EXPECT_NE(dynamic_cast<expl::DormandPrince45*>(solver.get()), nullptr);
}

} // namespace
} // namespace frelsim::integrate::factory
