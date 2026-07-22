#include "PIDController.hpp"
#include "../factory/ModelFactory.hpp"

namespace frelsim::model::algorithm {

PIDController::PIDController(sim::proto::SimulationDescription const& simDescription)
    : Model(simDescription), period_(simDescription.task().period()) {
    continuousStates_ = State(0); // no continuous state - this is a discrete controller

    derivative_ = std::make_shared<std::function<State(State const&, double)>>(
        [](State const& y, double /*t*/) -> State {
            return State(y.size()); // trivially empty; never actually integrated
        });
}

void PIDController::update() {
    double const error = setpoint_ - measurement_;
    integral_ += error * period_;
    double const derivativeTerm = (period_ > 0.0) ? (error - previousError_) / period_ : 0.0;
    output_ = kp_ * error + ki_ * integral_ + kd_ * derivativeTerm;
    previousError_ = error;
}

Derivative const& PIDController::derivative() const {
    return derivative_;
}

Values PIDController::getOutputs(Identifiers ids) const {
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

void PIDController::setInputs(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "measurement") {
            measurement_ = value.asDouble();
        }
    }
}

Values PIDController::getParameters(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "kp") {
            values.push_back(type::core::Value::makeDouble(kp_));
        }
        else if (id.getName() == "ki") {
            values.push_back(type::core::Value::makeDouble(ki_));
        }
        else if (id.getName() == "kd") {
            values.push_back(type::core::Value::makeDouble(kd_));
        }
        else if (id.getName() == "setpoint") {
            values.push_back(type::core::Value::makeDouble(setpoint_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void PIDController::setParameters(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "kp") {
            kp_ = value.asDouble();
        }
        else if (id.getName() == "ki") {
            ki_ = value.asDouble();
        }
        else if (id.getName() == "kd") {
            kd_ = value.asDouble();
        }
        else if (id.getName() == "setpoint") {
            setpoint_ = value.asDouble();
        }
    }
}

FRELSIM_REGISTER_MODEL("PIDController", PIDController)

} // frelsim::model::algorithm
