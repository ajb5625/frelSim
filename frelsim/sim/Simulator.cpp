#include "Simulator.hpp"
#include <algorithm> // for min()

namespace frelsim::sim {

Simulator::Simulator(const frelsim::sim::proto::System& system) : system_(system)
                                                                , simulationTime_(-1.0) 
                                                                , tFinal_(system_.stop_time())
                                                                , maxStepSize_(system_.max_step_size())
                                                                , isStopRequested_(false)  {}

Simulator::~Simulator() {

}

/**
 * \brief sim()
 * We step to the minimum of the maxStepSize and the next discrete time.
 *
 * We do this because some components could contain continuous functions which
 * need to be integrated more frequently. This is the intention of the maxStepSize parameter.
 * A discrete event may occur infrequently, so we rely on the user to know how frequently their
 * system needs an integration update to continuous states.
*/
void Simulator::sim() {
    initialize();
    while (simulationTime_ < tFinal_) {
        double stepTo = std::min(maxStepSize_, scheduler_->getNextDiscreteTime(simulationTime_));
        step(stepTo);
    }
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