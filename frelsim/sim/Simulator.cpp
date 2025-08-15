#include "Simulator.hpp"


namespace frelsim::sim {

Simulator::Simulator(double tFinal) : tFinal_(tFinal) {}

Simulator::~Simulator() {
}

void Simulator::sim() {
    initialize();
    step(tFinal_);
    terminate();
}

bool Simulator::step(double stopTime) {

    return false;
}

void Simulator::initialize() {

}

void Simulator::terminate() {

}

void Simulator::pause() {

}

void Simulator::resume() {

}
} // frelsim::sim