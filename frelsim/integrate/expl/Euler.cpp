#include "Euler.hpp"


namespace frelsim::integrate::expl {

Euler::Euler(double tFinal) : tFinal_(tFinal) {}


Euler::~Euler() {}

bool Euler::step(double stopTime) {
    (void)stopTime;
    return false;
}




} // namespace frelsim::integrate::expl