#include "DiscreteSimulation.hpp"

namespace frelsim::simulation {

DiscreteSimulation::DiscreteSimulation(double sampleTime) : sampleTime_(sampleTime) {}

void DiscreteSimulation::stepUntil(double stopTime) {

}

SimulationType DiscreteSimulation::type() {
    return SimulationType::Discrete;
}

void DiscreteSimulation::update() {}


} // frelsim::simulation 