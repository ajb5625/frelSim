#include "ModelFactory.hpp"
#include <map>

namespace frelsim::model::factory {

namespace {

// Function-local static: guarantees the registry exists before the first
// registerModel() call regardless of static-initialization order across
// translation units (the "static initialization order fiasco").
std::map<std::string, ModelCreator>& registry() {
    static std::map<std::string, ModelCreator> instance;
    return instance;
}

} // namespace

bool registerModel(std::string const& componentName, ModelCreator creator) {
    registry()[componentName] = std::move(creator);
    return true;
}

std::unique_ptr<core::Model> createModel(sim::proto::SimulationDescription const& simDescription) {
    std::string const& componentName = simDescription.model_spec().component_name();
    auto it = registry().find(componentName);
    if (it == registry().end()) {
        return nullptr;
    }
    return it->second(simDescription);
}

} // frelsim::model::factory
