#include "BangBangController.hpp"
#include "../factory/ModelFactory.hpp"

namespace frelsim::model::algorithm {

BangBangController::BangBangController(sim::proto::SimulationDescription const& simDescription) : Model(simDescription) {
    continuousStates_ = State(0); // no continuous state - this is a discrete controller

    derivative_ = std::make_shared<std::function<State(State const&, double)>>(
        [](State const& y, double /*t*/) -> State {
            return State(y.size()); // trivially empty; never actually integrated
        });
}

void BangBangController::update() {
    double const error = setpoint_ - measurement_;
    output_ = (error >= 0.0) ? amplitude_ : -amplitude_;
}

Derivative const& BangBangController::derivative() const {
    return derivative_;
}

Values BangBangController::getOutputs(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "output") {
            values.push_back(type::core::Value::makeDouble(output_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void BangBangController::setInputs(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "measurement") {
            measurement_ = value.asDouble();
        }
    }
}

Values BangBangController::getInputs(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "measurement") {
            values.push_back(type::core::Value::makeDouble(measurement_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

Values BangBangController::getParameters(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "setpoint") {
            values.push_back(type::core::Value::makeDouble(setpoint_));
        }
        else if (id.getName() == "amplitude") {
            values.push_back(type::core::Value::makeDouble(amplitude_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void BangBangController::setParameters(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "setpoint") {
            setpoint_ = value.asDouble();
        }
        else if (id.getName() == "amplitude") {
            amplitude_ = value.asDouble();
        }
    }
}

FRELSIM_REGISTER_MODEL("BangBangController", BangBangController)

} // frelsim::model::algorithm
