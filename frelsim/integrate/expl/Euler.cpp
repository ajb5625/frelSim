#include "Euler.hpp"
#include "../factory/SolverFactory.hpp"


namespace frelsim::integrate::expl {

Euler::Euler(double stopTime, double stepSize, const Derivative& f) : FixedStepSolver(stopTime, stepSize, f) {}


Euler::~Euler() {}

void Euler::singleStep(State& y0, double currentTime, double dt) {
    State dydt = (*f_)(y0, currentTime);
    for (Eigen::Index idx = 0; idx < y0.size(); idx++) {
        y0[idx] = y0[idx] + dt * dydt[idx];
    }
}

} // namespace frelsim::integrate::expl

FRELSIM_REGISTER_SOLVER(::frelsim::sim::proto::SolverType::Euler,
    [](double stopTime, ::frelsim::integrate::factory::SolverConfig const& config,
       ::frelsim::Derivative const& f, ::frelsim::JacobianFunction const&) {
        return std::make_unique<frelsim::integrate::expl::Euler>(stopTime, config.stepSize, f);
    });
