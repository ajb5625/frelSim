#include "Overseer.hpp"
#include <stdexcept>

namespace frelsim::overseer {

Overseer::Overseer(std::string const& configPath) : system_(compiler::Compiler().compile(configPath)) {}

Overseer::Overseer(sim::proto::System system) : system_(std::move(system)) {}

Overseer::~Overseer() = default;

void Overseer::initialize() {
    simulator_ = std::make_unique<simulator::Simulator>(linker_.link(system_));
    simulator_->initialize();
}

void Overseer::sim() {
    requireInitialized().sim();
}

bool Overseer::step(double stopTime) {
    return requireInitialized().step(stopTime);
}

void Overseer::pause() {
    requireInitialized().pause();
}

void Overseer::resume() {
    requireInitialized().resume();
}

void Overseer::terminate() {
    requireInitialized().terminate();
}

Values Overseer::get(std::string const& simulationKey, Identifiers const& ids) const {
    return requireInitialized().get(simulationKey, ids);
}

double Overseer::simulationTime() const {
    return requireInitialized().simulationTime();
}

simulator::Simulator& Overseer::requireInitialized() {
    if (!simulator_) {
        throw std::logic_error("Overseer: not initialized - call initialize() first");
    }
    return *simulator_;
}

simulator::Simulator const& Overseer::requireInitialized() const {
    if (!simulator_) {
        throw std::logic_error("Overseer: not initialized - call initialize() first");
    }
    return *simulator_;
}

} // namespace frelsim::overseer
