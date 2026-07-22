#pragma once
#include <functional>
#include <vector>
#include <map>
#include <memory>

#include "../util/Aliases.hpp"
#include "../event/Event.hpp"
#include "../adapt/SimAdapter.hpp"
#include "frelsim/proto/Simulation.pb.h"

/**
 * \file Simulation.hpp
 * \brief A Simulation is one unit of a co-simulatable entity.
 * This could be a frelSim task, FMI3 co-simulation, or Code.
 * \author AJ
 */

namespace frelsim::simulation {

class Simulation final {

    public:

        Simulation(const sim::proto::SimulationDescription& simDescription);

        ~Simulation();

        /**
         * \brief The latest time this Simulation can safely be stepped to
         * without missing a discrete task or zero-crossing event, capped at
         * maxTime. Forwards to the underlying SimAdapter.
         */
        double guaranteeUntil(double maxTime);

        /**
         * \brief Advance this simulation's internal time.
         * Update any states this simulation holds.
         * \param stopTime The time the simulation should advance to.
         */
         bool stepUntil(double stopTime);

         Values get(const Identifiers& identifiers) const;

         void set(SetOperations& setOperations);

    private:
        /// @brief The adapter implementation to generalize different kinds of Simulations.
        std::unique_ptr<adapt::SimAdapter> simAdapter_;

        /// \brief Metadata and setup info for the simulation.
        sim::proto::SimulationDescription simDescription_;

};


} // frelsim::simulation
