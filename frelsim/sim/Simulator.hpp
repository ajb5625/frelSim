#pragma once
#include <memory>
#include "../integrate/core/Solver.hpp"

namespace frelsim::sim {

class Simulator final {

public:
    Simulator(double tFinal);

    ~Simulator();

    void sim();

    bool step(double stopTime);

    void initialize();

    void terminate();

    void pause();

    void resume();


private:

    std::unique_ptr<integrate::core::Solver> solver_;

    double tFinal_;
};
}