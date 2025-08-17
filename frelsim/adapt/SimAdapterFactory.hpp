#include "SimAdapter.hpp"
#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::adapt {

/**
 * \file SimAdapterFactory.hpp
 * \brief Use this free function to create a simulation adapter of types
 * frelsim component, FMU, or Code.
 * \todo Make this use cppmicroserivces to load FMI dependencies dynamically.
 */

std::unique_ptr<SimAdapter> createSimAdapter(const sim::proto::SimulationDescription& simDescription);

}