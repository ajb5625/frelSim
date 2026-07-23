#pragma once

#include <memory>
#include <string>
#include "../compiler/Compiler.hpp"
#include "../linker/Linker.hpp"
#include "../recorder/Recorder.hpp"
#include "../simulator/Simulator.hpp"
#include "frelsim/proto/System.pb.h"

namespace frelsim::overseer {

/**
 * \file Overseer.hpp
 * \brief The single entry point to a simulation: owns all three stages
 * (compile a config into a System, link it into constructed+validated
 * Simulations, then execute it) and exposes both ways of running it - to
 * completion, or one externally-driven step at a time. This is what the sim
 * executable runner and, later, the Simulator gRPC service/REST layer sit
 * on top of; see docs/PROGRESS.md for how Overseer's method names mirror
 * Simulator.proto's (currently unimplemented) service one-to-one.
 */
class Overseer final {
    public:
        /**
         * \brief Stage 1 from a config file: runs Compiler itself. This is
         * the constructor a caller with only a file path (e.g. the sim
         * executable runner) uses - Overseer owns parsing, so callers never
         * need to touch Compiler directly.
         */
        explicit Overseer(std::string const& configPath);

        /**
         * \brief Stage 1 already done by the caller (e.g. a future gRPC
         * handler unpacking CreateSimulatorRequest.system - protobuf
         * already deserialized it off the wire, so there's nothing left
         * for Compiler to do). Skips Compiler entirely.
         */
        explicit Overseer(sim::proto::System system);

        ~Overseer();

        /**
         * \brief Stage 2 (link) plus bringing the Simulator to its ready
         * state (stage 3 setup). Must be called once before sim()/step().
         */
        void initialize();

        /// \brief Run-to-completion mode: steps until the simulation ends.
        void sim();

        /**
         * \brief Single-step mode: advance to stopTime and return whether
         * the whole simulation has finished. For a gRPC/REST caller or a
         * script driving the simulation one call at a time, instead of
         * sim().
         */
        bool step(double stopTime);

        void pause();

        void resume();

        void terminate();

        /**
         * \brief Inspect the current value(s) of one composed Simulation.
         * \throws std::out_of_range if simulationKey names no composed
         * Simulation.
         */
        Values get(std::string const& simulationKey, Identifiers const& ids) const;

        /// \brief Current global simulation time (debug/diagnostic use).
        double simulationTime() const;

        /**
         * \brief Writes the time series recorded so far (see
         * System.logged_outputs) to path as CSV - see
         * frelsim/recorder/Recorder.hpp for the format. sim() already calls
         * this automatically at the end of a run if System.log_path is set;
         * a step()-driven caller should call this explicitly once it's
         * done, since step() itself doesn't rewrite the file on every call.
         * \throws std::logic_error if System.logged_outputs was empty (
         * nothing was ever recorded).
         */
        void writeLog(std::string const& path) const;

    private:
        /// \brief Constructs recorder_ and wires it as the Simulator's step
        /// observer, if system_.logged_outputs() is non-empty. No-op
        /// otherwise, so a System with nothing to log pays nothing for it.
        void setUpRecordingIfConfigured();


        /**
         * \brief The Simulator initialize() constructed, or throws
         * std::logic_error if initialize() hasn't run yet (simulator_ is
         * still null).
         */
        simulator::Simulator& requireSimulator();
        simulator::Simulator const& requireSimulator() const;

        sim::proto::System system_;
        linker::Linker linker_;
        std::unique_ptr<simulator::Simulator> simulator_;
        std::unique_ptr<recorder::Recorder> recorder_;
};

} // namespace frelsim::overseer
