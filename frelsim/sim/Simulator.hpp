#pragma once
#include <memory>
#include "../simulation/Simulation.hpp"
#include "../task/Task.hpp"

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

    std::map<std::string, std::unique_ptr<simulation::Simulation>> idToSimulation_;

    double simulationTime_;

    double tFinal_;
};
}