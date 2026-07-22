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
    requireSimulator().sim();
}

bool Overseer::step(double stopTime) {
    return requireSimulator().step(stopTime);
}

void Overseer::pause() {
    requireSimulator().pause();
}

void Overseer::resume() {
    requireSimulator().resume();
}

void Overseer::terminate() {
    requireSimulator().terminate();
}

Values Overseer::get(std::string const& simulationKey, Identifiers const& ids) const {
    return requireSimulator().get(simulationKey, ids);
}

double Overseer::simulationTime() const {
    return requireSimulator().simulationTime();
}

simulator::Simulator& Overseer::requireSimulator() {
    if (!simulator_) {
        throw std::logic_error("Overseer: not initialized - call initialize() first");
    }
    return *simulator_;
}

simulator::Simulator const& Overseer::requireSimulator() const {
    if (!simulator_) {
        throw std::logic_error("Overseer: not initialized - call initialize() first");
    }
    return *simulator_;
}

} // namespace frelsim::overseer
