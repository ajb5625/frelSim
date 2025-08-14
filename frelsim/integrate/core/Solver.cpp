#include "Solver.hpp"

namespace frelsim::integrate::core {
    
Solver::Solver(double stopTime
             , double stepSize
             , const Derivative f
             , const JacobianFunction jf) : stopTime_(stopTime)
                                          , stepSize_(stepSize)
                                          , f_(f)
                                          , jacobianFunction_(jf) {}

Solver::~Solver() = default;


} // namespace frelsim::integrate::core