#include <gtest/gtest.h>
#include <cmath>
#include "frelsim/integrate/expl/DormandPrince45.hpp"
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
    expl::Euler solver(/*stopTime=*/1.0, /*stepSize=*/0.001, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    solver.step(y, 0.0, 1.0);

    // Euler is first-order accurate; a 0.001 step over 1 second should still
    // land within a loose but meaningful tolerance of the true e^-1.
    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-2);
}

TEST(RungeKutta4Test, ApproximatesExponentialDecayTightly) {
    expl::RungeKutta4 solver(/*stopTime=*/1.0, /*stepSize=*/0.01, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    solver.step(y, 0.0, 1.0);

    // RK4 is fourth-order accurate, so even a coarser 0.01 step should be
    // far more accurate than Euler's finer 0.001 step above.
    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-6);
}

TEST(RungeKutta4Test, IntegratesTheActualRequestedIntervalNotJustOneFixedStep) {
    // Regression test: Solver::step used to take exactly one step of its
    // fixed construction-time size regardless of how far apart currentTime
    // and targetTime actually were - correct only when a caller happened to
    // always ask in exact multiples of that size, silently wrong (badly
    // under- or over-integrating) the moment it didn't. This is exactly
    // what happened composing PIDController with MassSpringDamper through
    // Simulator, where the common step size varies (see docs/PROGRESS.md).
    // Ask for the whole interval in one call with a step size that doesn't
    // evenly divide it, forcing internal sub-stepping with a smaller final
    // step - if step() only took one stepSize_-sized step, this would land
    // nowhere near targetTime and miss badly.
    expl::RungeKutta4 solver(/*stopTime=*/1.0, /*stepSize=*/0.03, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    solver.step(y, 0.0, 1.0);

    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-5);
}

TEST(RungeKutta4Test, ReachesExactlyTheSameTargetAcrossSeveralSmallerCalls) {
    // A caller (Model::stepUntil) may reach the same overall target through
    // several smaller step() calls instead of one big one (e.g. landing on
    // intermediate discrete/event boundaries) - this should integrate to
    // essentially the same result as covering the interval in one call.
    expl::RungeKutta4 solver(/*stopTime=*/1.0, /*stepSize=*/0.01, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    double t = 0.0;
    for (double target : {0.3, 0.55, 0.7, 1.0}) {
        solver.step(y, t, target);
        t = target;
    }

    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-6);
}

TEST(BackwardEulerTest, ApproximatesExponentialDecay) {
    impl::BackwardEuler solver(/*stopTime=*/1.0, /*stepSize=*/0.001, exponentialDecayDerivative(), exponentialDecayJacobian());
    State y(1);
    y[0] = 1.0;

    solver.step(y, 0.0, 1.0);

    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-2);
}

TEST(DormandPrince45Test, ApproximatesExponentialDecayVeryTightly) {
    expl::DormandPrince45 solver(/*stopTime=*/1.0
                                , /*relativeTolerance=*/1e-8
                                , /*absoluteTolerance=*/1e-10
                                , /*minStepSize=*/1e-8
                                , /*maxStepSize=*/0.1
                                , exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    solver.step(y, 0.0, 1.0);

    // A 5th-order adaptive method at tight tolerance should comfortably beat
    // the fixed-step solvers above despite far fewer, larger steps.
    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-9);
}

TEST(DormandPrince45Test, IntegratesATimeVaryingDerivativeCorrectly) {
    // dy/dt = t has the closed-form solution y(t) = y0 + t^2/2. Exponential
    // decay above never exercises the Butcher tableau's time-node
    // coefficients (c2..c6), since its derivative ignores t entirely - this
    // specifically catches a transcription error in those.
    Derivative rampDerivative = std::make_shared<std::function<State(State const&, double)>>(
        [](State const&, double t) -> State {
            State dydt(1);
            dydt[0] = t;
            return dydt;
        });

    expl::DormandPrince45 solver(/*stopTime=*/2.0
                                , /*relativeTolerance=*/1e-8
                                , /*absoluteTolerance=*/1e-10
                                , /*minStepSize=*/1e-8
                                , /*maxStepSize=*/0.5
                                , rampDerivative);
    State y(1);
    y[0] = 0.0;

    solver.step(y, 0.0, 2.0);

    EXPECT_NEAR(y[0], 2.0, 1e-8); // 0 + 2^2/2 = 2.0
}

TEST(DormandPrince45Test, ReachesExactlyTheSameTargetAcrossSeveralSmallerCalls) {
    expl::DormandPrince45 solver(/*stopTime=*/1.0
                                , /*relativeTolerance=*/1e-8
                                , /*absoluteTolerance=*/1e-10
                                , /*minStepSize=*/1e-8
                                , /*maxStepSize=*/0.2
                                , exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    double t = 0.0;
    for (double target : {0.3, 0.55, 0.7, 1.0}) {
        solver.step(y, t, target);
        t = target;
    }

    EXPECT_NEAR(y[0], std::exp(-1.0), 1e-8);
}

TEST(DormandPrince45Test, LooserToleranceTakesFewerDerivativeEvaluations) {
    // Demonstrates the adaptation actually engages, not just accepting
    // every step at some fixed size: a looser tolerance should let the
    // solver take bigger steps and therefore call the derivative function
    // fewer times to cover the same interval.
    auto makeCountingDerivative = [](int& count) {
        return std::make_shared<std::function<State(State const&, double)>>(
            [&count](State const& y, double) -> State {
                ++count;
                State dydt(1);
                dydt[0] = -y[0];
                return dydt;
            });
    };

    int tightCount = 0;
    expl::DormandPrince45 tightSolver(1.0, /*relativeTolerance=*/1e-10, /*absoluteTolerance=*/1e-12, 1e-8, 0.5, makeCountingDerivative(tightCount));
    State yTight(1);
    yTight[0] = 1.0;
    tightSolver.step(yTight, 0.0, 1.0);

    int looseCount = 0;
    expl::DormandPrince45 looseSolver(1.0, /*relativeTolerance=*/1e-3, /*absoluteTolerance=*/1e-5, 1e-8, 0.5, makeCountingDerivative(looseCount));
    State yLoose(1);
    yLoose[0] = 1.0;
    looseSolver.step(yLoose, 0.0, 1.0);

    EXPECT_LT(looseCount, tightCount);
}

TEST(SolverTest, StepReturnsTrueOnceSimulationTimeReachesStopTime) {
    expl::Euler solver(/*stopTime=*/1.0, /*stepSize=*/0.5, exponentialDecayDerivative());
    State y(1);
    y[0] = 1.0;

    EXPECT_FALSE(solver.step(y, 0.0, 0.5));
    EXPECT_TRUE(solver.step(y, 0.5, 1.0));
}

} // namespace
} // namespace frelsim::integrate
