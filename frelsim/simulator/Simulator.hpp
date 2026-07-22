#pragma once
#include <atomic>
#include <map>
#include <memory>
#include "../simulation/Simulation.hpp"
#include "../linker/Linker.hpp"
#include "frelsim/proto/System.pb.h"

/**
 * \file Simulator.hpp
 * \brief The Simulator serves as the entry point for simulating a dynamic system.
 * The Simulator's responsibities are to advance global time, route values
 * between composed Simulations, and dispatch tasks.
 * \author AJ
 */

namespace frelsim::simulator {

class Simulator final {

public:

    /**
     * \brief Simulator Constructor
     * \param linkedSystem Every composed Simulation, already constructed,
     * initial-parameterized, and composition-validated by Linker, plus the
     * System metadata (stop_time/max_step_size/composition) still needed
     * for routing and time advancement. Simulator no longer constructs or
     * validates anything itself - that's Linker's job, run before a
     * Simulator ever exists (see Overseer, which owns invoking Linker).
     */
    explicit Simulator(linker::LinkedSystem linkedSystem);

    /**
     * \brief Simulator Destructor
     */
    ~Simulator();

    /**
     * \brief Run a full simulation and steps until tFinal.
     */
    void sim();

    /**
     * \brief Advance the global simulation time by one communication
     * interval, capped at the earliest of: stopTime, the System's own
     * max_step_size, and every composed Simulation's own guaranteeUntil
     * horizon (a conservative, non-iterating lock-step master algorithm -
     * see docs/PROGRESS.md, orchestration track). Routes values between
     * Simulations (per the System's composition graph, using each source's
     * value from *before* this step - one step of lag, not solved to a
     * fixed point) before stepping them.
     * \param stopTime The time the simulation should step until.
     * \returns true once the overall simulation has reached tFinal.
     */
    bool step(double stopTime);

    /**
     * \brief Bring the simulator to its ready state (reset global time).
     * Construction and composition validation already happened at link
     * time (see Linker), before this Simulator existed.
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

    /**
     * \brief Inspect the current value(s) of one composed Simulation, e.g.
     * for a client watching a running/paused simulation from outside.
     * \param simulationKey The domain segment of that Simulation's
     * RoutedSimulation.simulation identifier (its key in the composition).
     * \throws std::out_of_range if simulationKey names no composed Simulation.
     */
    Values get(std::string const& simulationKey, Identifiers const& ids) const;

    /// \brief Current global simulation time (debug/diagnostic use).
    double simulationTime() const { return simulationTime_; }


private:

    /**
     * \brief Wait while paused.
     */
    void wait();

    /**
     * \brief For every wired edge (RoutedSimulation.source -> .destinations)
     * in the composition, fetch source's current value and set it on every
     * destination.
     */
    void route();

    /// @brief The serialization of the system we are simulating.
    sim::proto::System system_;

    /// @brief Every composed Simulation, keyed by its RoutedSimulation's
    /// `simulation` identifier's domain - the same key RoutedSimulation.source/
    /// destinations' domain must reference to route to/from it.
    std::map<std::string, std::unique_ptr<simulation::Simulation>> idToSimulation_;

    /// @brief Current global simulation time.
    double simulationTime_;

    /// @brief Simulation stop time.
    double tFinal_;

    /// @brief Max step size of system provided by user.
    double maxStepSize_;

    /// @brief set to true when the client requested a pause.
    std::atomic<bool> isStopRequested_;
};
}
