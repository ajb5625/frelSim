#include "LongitudinalVehicle.hpp"
#include "../factory/ModelFactory.hpp"
#include <cmath>

namespace frelsim::model::algorithm {

LongitudinalVehicle::LongitudinalVehicle(sim::proto::SimulationDescription const& simDescription) : Model(simDescription) {
    continuousStates_ = State(2);
    continuousStates_[0] = 0.0; // initial position (m)
    continuousStates_[1] = 0.0; // initial velocity (m/s)

    derivative_ = std::make_shared<std::function<State(State const&, double)>>(
        [this](State const& y, double /*t*/) -> State {
            double const velocity = y[1];
            State dydt(2);
            dydt[0] = velocity;
            dydt[1] = (throttle_ - dragCoefficient_ * velocity * std::abs(velocity) - rollingResistance_ * velocity) / mass_;
            return dydt;
        });

    jacobian_ = std::make_shared<std::function<Matrix(State const&, double)>>(
        [this](State const& y, double /*t*/) -> Matrix {
            double const velocity = y[1];
            Matrix jacobian(2, 2);
            jacobian(0, 0) = 0.0;
            jacobian(0, 1) = 1.0;
            jacobian(1, 0) = 0.0;
            jacobian(1, 1) = (-2.0 * dragCoefficient_ * std::abs(velocity) - rollingResistance_) / mass_;
            return jacobian;
        });
}

void LongitudinalVehicle::update() {
    // No discrete states; nothing to update.
}

Derivative const& LongitudinalVehicle::derivative() const {
    return derivative_;
}

JacobianFunction const& LongitudinalVehicle::jacobian() const {
    return jacobian_;
}

Values LongitudinalVehicle::getOutputs(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "position") {
            values.push_back(type::core::Value::makeDouble(continuousStates_[0]));
        }
        else if (id.getName() == "velocity") {
            values.push_back(type::core::Value::makeDouble(continuousStates_[1]));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void LongitudinalVehicle::setInputs(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "throttle") {
            throttle_ = value.asDouble();
        }
    }
}

Values LongitudinalVehicle::getInputs(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "throttle") {
            values.push_back(type::core::Value::makeDouble(throttle_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

Values LongitudinalVehicle::getParameters(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "mass") {
            values.push_back(type::core::Value::makeDouble(mass_));
        }
        else if (id.getName() == "dragCoefficient") {
            values.push_back(type::core::Value::makeDouble(dragCoefficient_));
        }
        else if (id.getName() == "rollingResistance") {
            values.push_back(type::core::Value::makeDouble(rollingResistance_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void LongitudinalVehicle::setParameters(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "mass") {
            mass_ = value.asDouble();
        }
        else if (id.getName() == "dragCoefficient") {
            dragCoefficient_ = value.asDouble();
        }
        else if (id.getName() == "rollingResistance") {
            rollingResistance_ = value.asDouble();
        }
    }
}

FRELSIM_REGISTER_MODEL("LongitudinalVehicle", LongitudinalVehicle)

} // frelsim::model::algorithm
