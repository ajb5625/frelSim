#include "ModelFactory.hpp"
#include "../algorithm/BouncingBall.hpp"

namespace frelsim::model::factory {

std::unique_ptr<core::Model> createModel(sim::proto::SimulationDescription const& simDescription) {
    std::string const& componentName = simDescription.model_spec().component_name();
    if (componentName == "BouncingBall") {
        return std::make_unique<algorithm::BouncingBall>(simDescription);
    }
    return nullptr;
}

} // frelsim::model::factory
