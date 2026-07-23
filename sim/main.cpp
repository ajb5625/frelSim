#include <iostream>
#include "frelsim/overseer/Overseer.hpp"

/**
 * \file main.cpp
 * \brief The sim executable runner (task #10): loads a JSON config through
 * Overseer and runs it to completion. Deliberately thin - Overseer already
 * owns all three simulation stages (compile the config, link the
 * composition, execute it) internally, so this file has no parsing or
 * validation logic of its own; it only loads Overseer and drives it.
 * Interactive/single-step control (Overseer::step()) is exposed for a
 * future gRPC/REST caller, not this executable, which only ever needs
 * run-to-completion mode.
 */
int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config.json>\n";
        return 1;
    }

    try {
        frelsim::overseer::Overseer overseer(argv[1]);
        overseer.initialize();
        overseer.sim();
        std::cout << "Simulation complete at t=" << overseer.simulationTime() << "\n";
    } catch (std::exception const& e) {
        std::cerr << "frelsim_sim: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
