#include "BackwardEuler.hpp"


namespace frelsim::integrate::impl {

BackwardEuler::BackwardEuler(double stopTime
                        , double stepSize
                        , const Derivative f
                        , const JacobianFunction jf) : FixedStepSolver(stopTime, stepSize, f, jf)  {}

void BackwardEuler::singleStep(State& y0, double currentTime, double dt) {
    double const targetTime = currentTime + dt;

    // initial guess for y1
    State y1 = y0;
    // set initial guess explicitly with Euler formula
    y1 += dt * (*f_)(y0, currentTime);

    // The goal of this Newton-Raphson iteration is to solve for the next state, y1.
    // To reach y1, the residual function g must be approximately zero.
    for (int iteration = 0; iteration < maxIterations_; iteration++) {
        // first compute the residual.
        State g = y1 - y0 - dt * (*f_)(y1, targetTime);

        // If residual norm is close to 0, we are done
        if (g.norm() < SolverTolerance) {
            break;
        }

        // get Jacobian of residual wrt y
        Matrix jacobian = Matrix::Identity(y1.size(), y1.size()) - dt * (*jacobianFunction_)(y1, targetTime);

        // solve the linear system for delta in our state
        State delta = jacobian.fullPivLu().solve(g);

        y1 -= delta;

        // if delta is sufficiently small, we are done
        if (delta.norm() < SolverTolerance) {
            break;
        }
    }

    // Write output
    y0 = y1;
}


}
