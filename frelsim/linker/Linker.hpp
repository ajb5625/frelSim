#pragma once

#include <map>
#include <memory>
#include "../simulation/Simulation.hpp"
#include "frelsim/proto/System.pb.h"

namespace frelsim::linker {

/**
 * \brief The output of linking: every composed Simulation, already
 * constructed and initial-parameterized, keyed by its RoutedSimulation's
 * `simulation` identifier's domain - the same key RoutedSimulation.source/
 * destinations' domain must reference to route to/from it. `system` is
 * carried alongside since the executing Simulator still needs its
 * stop_time/max_step_size/composition for routing and time advancement.
 */
struct LinkedSystem {
    sim::proto::System system;
    std::map<std::string, std::unique_ptr<simulation::Simulation>> simulations;
};

/**
 * \file Linker.hpp
 * \brief Stage 2 of the config -> link -> execute pipeline (see Overseer):
 * constructs every composed Simulation from a System's composition and
 * validates the wiring between them, so an unknown simulation reference or
 * a type mismatch between a source's output and a destination's input
 * surfaces as a clear error at link time - before a Simulator ever exists -
 * rather than silently or deep into the run loop. This was previously
 * Simulator's job (Simulator::initialize()/validateComposition()); moved
 * out since composition validation isn't really a runtime/execution
 * concern.
 */
class Linker {
    public:
        /**
         * \brief Constructs every composed Simulation from `system`'s
         * composition, applies each RoutedSimulation's initial_parameters,
         * and dry-run validates every wired edge (fetch from source, apply
         * to each destination) once.
         * \throws std::invalid_argument on an unknown source/destination
         * simulation reference, a source producing no value, or a type
         * mismatch surfacing from a destination's set().
         */
        LinkedSystem link(sim::proto::System const& system) const;
};

} // namespace frelsim::linker
