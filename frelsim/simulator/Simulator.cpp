#include "Simulator.hpp"
#include <algorithm> // for min()
#include <stdexcept>
#include <thread> // for sleep

namespace frelsim::simulator {

namespace {

util::Identifier toUtilIdentifier(sim::proto::Identifier const& protoId) {
    util::Identifier id;
    id.setDomain(protoId.domain());
    id.setScope(protoId.scope());
    id.setName(protoId.name());
    return id;
}

bool isSet(sim::proto::Identifier const& protoId) {
    return !protoId.domain().empty() || !protoId.scope().empty() || !protoId.name().empty();
}

} // namespace

Simulator::Simulator(linker::LinkedSystem linkedSystem) : system_(std::move(linkedSystem.system))
                                                          , idToSimulation_(std::move(linkedSystem.simulations))
                                                          , simulationTime_(0.0)
                                                          , tFinal_(system_.stop_time())
                                                          , maxStepSize_(system_.max_step_size())
                                                          , isStopRequested_(false)  {}

Simulator::~Simulator() = default;

void Simulator::sim() {
    initialize();
    bool finished = false;
    while (!finished) {
        finished = step(tFinal_);
        if (isStopRequested_) {
            wait();
        }
    }
    terminate();
}

bool Simulator::step(double stopTime) {
    double const target = std::min(stopTime, tFinal_);
    double horizon = std::min(simulationTime_ + maxStepSize_, target);
    for (auto& [key, sim] : idToSimulation_) {
        horizon = std::min(horizon, sim->guaranteeUntil(horizon));
    }

    route();

    for (auto& [key, sim] : idToSimulation_) {
        sim->stepUntil(horizon);
    }
    simulationTime_ = horizon;

    return simulationTime_ >= tFinal_;
}

void Simulator::route() {
    for (auto const& routed : system_.composition()) {
        if (!isSet(routed.source())) {
            continue;
        }
        util::Identifier const sourceId = toUtilIdentifier(routed.source());
        Values const value = idToSimulation_.at(sourceId.getDomain())->get({sourceId});
        for (auto const& destProto : routed.destinations()) {
            util::Identifier const destId = toUtilIdentifier(destProto);
            SetOperations op{{destId, value[0]}};
            idToSimulation_.at(destId.getDomain())->set(op);
        }
    }
}

void Simulator::initialize() {
    simulationTime_ = 0.0;
}

Values Simulator::get(std::string const& simulationKey, Identifiers const& ids) const {
    return idToSimulation_.at(simulationKey)->get(ids);
}

void Simulator::terminate() {

}

void Simulator::pause() {
    isStopRequested_ = true;
}

void Simulator::resume() {
    isStopRequested_ = false;
}

void Simulator::wait() {
    while (isStopRequested_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
} // frelsim::simulator
