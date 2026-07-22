#include "Simulator.hpp"
#include "../type/marshal/Marshaler.hpp"
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

std::string describeIdentifier(sim::proto::Identifier const& protoId) {
    return protoId.domain() + "." + protoId.scope() + "." + protoId.name();
}

bool isSet(sim::proto::Identifier const& protoId) {
    return !protoId.domain().empty() || !protoId.scope().empty() || !protoId.name().empty();
}

} // namespace

Simulator::Simulator(const frelsim::sim::proto::System& system) : system_(system)
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
    idToSimulation_.clear();
    type::marshal::Marshaler const marshaler;
    for (auto const& routed : system_.composition()) {
        std::string const key = routed.simulation().domain();
        auto simulation = std::make_unique<simulation::Simulation>(routed.sim_description());

        if (routed.initial_parameters_size() > 0) {
            SetOperations ops;
            ops.reserve(static_cast<std::size_t>(routed.initial_parameters_size()));
            for (auto const& op : routed.initial_parameters()) {
                ops.emplace_back(toUtilIdentifier(op.id()), marshaler.protoToCpp(op.value()));
            }
            simulation->set(ops);
        }

        idToSimulation_[key] = std::move(simulation);
    }
    validateComposition();
    simulationTime_ = 0.0;
}

void Simulator::validateComposition() {
    for (auto const& routed : system_.composition()) {
        if (!isSet(routed.source())) {
            continue;
        }

        util::Identifier const sourceId = toUtilIdentifier(routed.source());
        auto sourceIt = idToSimulation_.find(sourceId.getDomain());
        if (sourceIt == idToSimulation_.end()) {
            throw std::invalid_argument(
                "Simulator: composition wiring error - unknown source simulation '" + sourceId.getDomain() + "'");
        }

        Values probe;
        try {
            probe = sourceIt->second->get({sourceId});
        } catch (std::exception const& e) {
            throw std::invalid_argument(
                "Simulator: composition wiring error reading source '" + describeIdentifier(routed.source()) + "': " + e.what());
        }
        if (probe.empty()) {
            throw std::invalid_argument(
                "Simulator: composition wiring error - source '" + describeIdentifier(routed.source()) + "' produced no value");
        }

        for (auto const& destProto : routed.destinations()) {
            util::Identifier const destId = toUtilIdentifier(destProto);
            auto destIt = idToSimulation_.find(destId.getDomain());
            if (destIt == idToSimulation_.end()) {
                throw std::invalid_argument(
                    "Simulator: composition wiring error - unknown destination simulation '" + destId.getDomain() + "'");
            }
            SetOperations op{{destId, probe[0]}};
            try {
                destIt->second->set(op);
            } catch (std::exception const& e) {
                throw std::invalid_argument(
                    "Simulator: composition wiring error setting destination '" + describeIdentifier(destProto) + "': " + e.what());
            }
        }
    }
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
