#include "DiscreteComponent.hpp"

namespace frelsim::model {

DiscreteComponent::DiscreteComponent(double sampleTime) : sampleTime_(sampleTime) {}

ComponentType DiscreteComponent::type() {
    return ComponentType::Discrete;
}

double DiscreteComponent::sampleTime() {
    return sampleTime_;
}

void DiscreteComponent::update() {}


} // frelsim::model 