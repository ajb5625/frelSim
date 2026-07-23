#include "Overseer.hpp"
#include <stdexcept>

namespace frelsim::overseer {

namespace {

util::Identifier toUtilIdentifier(sim::proto::Identifier const& protoId) {
    util::Identifier id;
    id.setDomain(protoId.domain());
    id.setScope(protoId.scope());
    id.setName(protoId.name());
    return id;
}

} // namespace

Overseer::Overseer(std::string const& configPath) : system_(compiler::Compiler().compile(configPath)) {}

Overseer::Overseer(sim::proto::System system) : system_(std::move(system)) {}

Overseer::~Overseer() = default;

void Overseer::initialize() {
    simulator_ = std::make_unique<simulator::Simulator>(linker_.link(system_));
    simulator_->initialize();
    setUpRecordingIfConfigured();
}

void Overseer::setUpRecordingIfConfigured() {
    if (system_.logged_outputs_size() == 0) {
        return;
    }

    std::vector<util::Identifier> watched;
    watched.reserve(static_cast<std::size_t>(system_.logged_outputs_size()));
    for (auto const& id : system_.logged_outputs()) {
        watched.push_back(toUtilIdentifier(id));
    }

    recorder_ = std::make_unique<recorder::Recorder>(std::move(watched));
    simulator_->setStepObserver([this](simulator::Simulator const& simulator) {
        recorder_->record(simulator.simulationTime(),
            [&simulator](std::string const& domain, Identifiers const& ids) {
                return simulator.get(domain, ids);
            });
    });
}

void Overseer::sim() {
    requireSimulator().sim();
    if (recorder_ && system_.has_log_path()) {
        recorder_->writeCsv(system_.log_path());
    }
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

void Overseer::writeLog(std::string const& path) const {
    if (!recorder_) {
        throw std::logic_error("Overseer: no logged_outputs configured in this System - nothing to write");
    }
    recorder_->writeCsv(path);
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
