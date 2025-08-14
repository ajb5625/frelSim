#include "BackwardEuler.hpp"


namespace frelsim::integrate::impl {

BackwardEuler::BackwardEuler(double stopTime
                        , double stepSize
                        , const Derivative f
                        , const JacobianFunction jf) : Solver(stopTime, stepSize, f, jf)  {}

bool BackwardEuler::step(State& y0, double simulationTime) {
    // initial guess for y1
    State y1 = y0;
    // set initial guess explicitly with Euler formula
    y1 += stepSize_ * (*f_)(y0, simulationTime);

    // The goal of this Newton-Raphson iteration is to solve for the next state, y1.
    // To reach y1, the residual function g must be approximately zero.
    for (int iteration = 0; iteration < maxIterations_; iteration++) {
        // first compute the residual.
        State g = y1 - y0 - stepSize_ * (*f_)(y1, simulationTime + stepSize_);

        // If residual norm is close to 0, we are done
        if (g.norm() < SolverTolerance) {
            break;
        }

        // get Jacobian of residual wrt y 
        Matrix jacobian = Matrix::Identity(y1.size(), y1.size()) - stepSize_ * (*jacobianFunction_)(y1, simulationTime + stepSize_);

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
    return util::almostEqual(simulationTime, stopTime_);
}


}