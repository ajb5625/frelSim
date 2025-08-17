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

// enum class SimulationType : int {
    // Continuous = 0,
    // Discrete = 1
// };

// using Parameter = double;

class Simulation final {

    public:

        Simulation(const sim::proto::SimulationDescription& simDescription);

        ~Simulation();

        /**
         * \brief Advance this simulation's internal time.
         * Update any states this simulation holds.
         * \param stopTime The time the simulation should advance to.
         */
         bool stepUntil(double stopTime);

         Values get(const Identifiers& identifiers) const;

         void set(SetOperations& setOperations);

        //  SimulationType type() = 0;
// 
        // std::vector<event::EventIndicator> eventIndicators() const;
// 
        //  void sampleTime();

    private:
        /**
         * \brief Update discrete states.
         */
        void update();

        /**
         * \brief Integrate continuous states.
         */
        void integrate();

        /// @brief The adapter implementation to generalize different kinds of Simulations.
        std::unique_ptr<adapt::SimAdapter> simAdapter_;

        /// \brief Metadata and setup info for the simulation.
        sim::proto::SimulationDescription simDescription_;

        /// \brief Continuous states vector to be integrated each step.
        State continuousStates_;

        /// \brief Discrete states vector to be updated each step.
        State discreteStates_;

        // std::vector<event::Event> events_;    

};


} // frelsim::simulation 