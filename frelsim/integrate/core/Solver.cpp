#include "Solver.hpp"

namespace frelsim::integrate::core {
    
Solver::Solver(double stopTime, double stepSize) : stopTime_(stopTime), stepSize_(stepSize) {}

Solver::~Solver() = default;


} // namespace frelsim::integrate::core