#pragma once
#include "../core/Model.hpp"
#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::model::factory {

std::unique_ptr<core::Model> createModel(sim::proto::SimulationDescription const& simDescription);

} // frelsim::model::factory
