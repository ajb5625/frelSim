#include <gtest/gtest.h>
#include <cmath>
#include "frelsim/integrate/expl/Euler.hpp"
#include "frelsim/integrate/expl/RungeKutta4.hpp"
#include "frelsim/integrate/impl/BackwardEuler.hpp"

namespace frelsim::integrate {
namespace {

// dy/dt = -y, y(0) = 1 has the closed-form solution y(t) = e^-t. Used to
// check each solver's accuracy against something known rather than just
// "did it run".
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

TEST(EulerTest, ApproximatesExponentialDecay) {
    double const stepSize = 0.001;
    double const totalTime = 1.0;
    int const numSteps = static_cast<int>(totalTime / stepSize);

    expl::Euler solver(totalTime, stepSize, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    double t = 0.0;
    for (int i = 0; i < numSteps; ++i) {
        solver.step(y, t);
        t += stepSize;
    }

    // Euler is first-order accurate; a 0.001 step over 1 second should still
    // land within a loose but meaningful tolerance of the true e^-1.
    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-2);
}

TEST(RungeKutta4Test, ApproximatesExponentialDecayTightly) {
    double const stepSize = 0.01;
    double const totalTime = 1.0;
    int const numSteps = static_cast<int>(totalTime / stepSize);

    expl::RungeKutta4 solver(totalTime, stepSize, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    double t = 0.0;
    for (int i = 0; i < numSteps; ++i) {
        solver.step(y, t);
        t += stepSize;
    }

    // RK4 is fourth-order accurate, so even a coarser 0.01 step should be
    // far more accurate than Euler's finer 0.001 step above.
    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-6);
}

TEST(BackwardEulerTest, ApproximatesExponentialDecay) {
    double const stepSize = 0.001;
    double const totalTime = 1.0;
    int const numSteps = static_cast<int>(totalTime / stepSize);

    impl::BackwardEuler solver(totalTime, stepSize, exponentialDecayDerivative(), exponentialDecayJacobian());
    State y(1);
    y[0] = 1.0;

    double t = 0.0;
    for (int i = 0; i < numSteps; ++i) {
        solver.step(y, t);
        t += stepSize;
    }

    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-2);
}

TEST(SolverTest, StepReturnsTrueOnceSimulationTimeReachesStopTime) {
    expl::Euler solver(/*stopTime=*/1.0, /*stepSize=*/0.5, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    EXPECT_FALSE(solver.step(y, 0.0));
    EXPECT_TRUE(solver.step(y, 1.0));
}

} // namespace
} // namespace frelsim::integrate
