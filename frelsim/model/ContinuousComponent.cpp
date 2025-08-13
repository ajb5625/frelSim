#include "ContinuousComponent.hpp"

namespace frelsim::model {

ComponentType ContinuousComponent::type() {
    return ComponentType::Continuous;
}

double ContinuousComponent::sampleTime() {
    return 0.0;
}

std::optional<Matrix> ContinuousComponent::jacobian() {
    return std::nullopt;
}


} // frelsim::model 