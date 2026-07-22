#pragma once
#include <cmath>

namespace frelsim::util {

constexpr double equalTolerance = 1e-12;

inline bool almostEqual(double a, double b) {
    return std::abs(a - b) < equalTolerance;
}


} // frelsim::util