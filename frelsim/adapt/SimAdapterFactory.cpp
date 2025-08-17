#include "SimAdapterFactory.hpp"
#include "../simulation/FrelsimAdapter.hpp"

namespace frelsim::adapt {

std::unique_ptr<SimAdapter> createSimAdapter(const sim::proto::SimulationDescription& simDescription) {
    switch (simDescription.cosim_target()) {
        case sim::proto::CosimulationTarget::FrelUnit:
            return std::make_unique<simulation::FrelsimAdapter>();
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