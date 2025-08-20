#include "SimAdapterFactory.hpp"
#include "../model/adapt/ModelAdapter.hpp"

namespace frelsim::adapt {

std::unique_ptr<SimAdapter> createSimAdapter(const sim::proto::SimulationDescription& simDescription) {
    switch (simDescription.cosim_target()) {
        case sim::proto::CosimulationTarget::Model:
            return std::make_unique<model::adapt::ModelAdapter>(simDescription);
        break;
        case sim::proto::CosimulationTarget::FMU:
            return nullptr;
        break;

        case sim::proto::CosimulationTarget::Code:
            return nullptr;
        break;
    }
}

} // frelsim::adapt