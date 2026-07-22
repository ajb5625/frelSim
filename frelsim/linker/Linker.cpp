#include "Linker.hpp"
#include "../type/marshal/Marshaler.hpp"
#include <stdexcept>

namespace frelsim::linker {

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

// Dry-run every wired edge once (fetch from source, apply to each
// destination) so an unknown simulation reference or a type mismatch
// between a source's output and a destination's input surfaces as a clear
// error here, not silently or deep into Simulator's run loop.
void validateComposition(sim::proto::System const& system
                        , std::map<std::string, std::unique_ptr<simulation::Simulation>> const& simulations) {
    for (auto const& routed : system.composition()) {
        if (!isSet(routed.source())) {
            continue;
        }

        util::Identifier const sourceId = toUtilIdentifier(routed.source());
        auto sourceIt = simulations.find(sourceId.getDomain());
        if (sourceIt == simulations.end()) {
            throw std::invalid_argument(
                "Linker: composition wiring error - unknown source simulation '" + sourceId.getDomain() + "'");
        }

        Values probe;
        try {
            probe = sourceIt->second->get({sourceId});
        } catch (std::exception const& e) {
            throw std::invalid_argument(
                "Linker: composition wiring error reading source '" + describeIdentifier(routed.source()) + "': " + e.what());
        }
        if (probe.empty()) {
            throw std::invalid_argument(
                "Linker: composition wiring error - source '" + describeIdentifier(routed.source()) + "' produced no value");
        }

        for (auto const& destProto : routed.destinations()) {
            util::Identifier const destId = toUtilIdentifier(destProto);
            auto destIt = simulations.find(destId.getDomain());
            if (destIt == simulations.end()) {
                throw std::invalid_argument(
                    "Linker: composition wiring error - unknown destination simulation '" + destId.getDomain() + "'");
            }
            SetOperations op{{destId, probe[0]}};
            try {
                destIt->second->set(op);
            } catch (std::exception const& e) {
                throw std::invalid_argument(
                    "Linker: composition wiring error setting destination '" + describeIdentifier(destProto) + "': " + e.what());
            }
        }
    }
}

} // namespace

LinkedSystem Linker::link(sim::proto::System const& system) const {
    LinkedSystem linked;
    linked.system = system;

    type::marshal::Marshaler const marshaler;
    for (auto const& routed : system.composition()) {
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

        linked.simulations[key] = std::move(simulation);
    }

    validateComposition(linked.system, linked.simulations);

    return linked;
}

} // namespace frelsim::linker
