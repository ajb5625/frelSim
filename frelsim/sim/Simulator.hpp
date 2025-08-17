#pragma once
#include <memory>
#include "../simulation/Simulation.hpp"
#include "../task/Task.hpp"
#include "../schedule/Scheduler.hpp"
#include "frelsim/proto/System.pb.h"

/**
 * \file Simulator.hpp
 * \brief The Simulator serves as the entry point for simulating a dynamic system.
 * The Simulator's responsibities are to advance global time and execute tasks.
 * \author AJ
 */

namespace frelsim::sim {

class Simulator final {

public:

    /**
     * \brief Simulator Constructor
     * \param system The serialization of the system of simulations.
     */
    Simulator(const frelsim::sim::proto::System& system);

    /**
     * \brief Simulator Destructor
     */
    ~Simulator();

    /**
     * \brief Run a full simulation and steps until tFinal.
     */
    void sim();

    /**
     * \brief Advance the global simulation time and dispatch tasks.
     * \param stopTime The time the simulation should step until. 
     */
    bool step(double stopTime);

    /**
     * \brief Create all tasks and put them into their initialized state.
     */
    void initialize();

    /**
     * \brief Tell the simulator that the work has completed.
     */
    void terminate();

    /**
     * \brief Put a running simulation into the paused state.
     */
    void pause();

    /**
     * \brief Continue execution of a paused simulation.
     */
    void resume();


private:

    /// @brief The serialization of the system we are simulating.
    proto::System system_;

    /// @brief  Map the task Id string to a co-simulation owned by the simulator.
    std::map<std::string, std::unique_ptr<simulation::Simulation>> idToSimulation_;

    /// @brief Scheduler is the engine to compute timing
    std::unique_ptr<schedule::Scheduler> scheduler_;

    /// @brief Current global simulation time.
    double simulationTime_;

    /// @brief Simulation stop time.
    double tFinal_;

    /// @brief Max step size of system provided by user.
    double maxStepSize_;

    /// @brief set to true when the client requested a pause.
    bool isStopRequested_;
};
}