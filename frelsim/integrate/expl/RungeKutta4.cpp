#include "RungeKutta4.hpp"
#include "../factory/SolverFactory.hpp"

namespace frelsim::integrate::expl {

RungeKutta4::RungeKutta4(double stopTime
                        , double stepSize
                        , const Derivative f) : FixedStepSolver(stopTime, stepSize, f) {}

RungeKutta4::~RungeKutta4() = default;

void RungeKutta4::singleStep(State& yn, double currentTime, double dt) {
    double const halfStep = dt / 2.0;
    State k1 = (*f_)(yn, currentTime);
    State k2 = (*f_)(yn + halfStep * k1, currentTime + halfStep);
    State k3 = (*f_)(yn + halfStep * k2, currentTime + halfStep);
    State k4 = (*f_)(yn + dt * k3, currentTime + dt);

    yn += (dt / 6.0) * (k1 + 2 * k2 + 2 * k3 + k4);
}



} // frelsim::integrate::expl

FRELSIM_REGISTER_SOLVER(::frelsim::sim::proto::SolverType::RungeKutta4,
    [](double stopTime, ::frelsim::integrate::factory::SolverConfig const& config,
       ::frelsim::Derivative const& f, ::frelsim::JacobianFunction const&) {
        return std::make_unique<frelsim::integrate::expl::RungeKutta4>(stopTime, config.stepSize, f);
    });
