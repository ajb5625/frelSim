#include "FirstOrderLag.hpp"
#include "../factory/ModelFactory.hpp"

namespace frelsim::model::algorithm {

FirstOrderLag::FirstOrderLag(sim::proto::SimulationDescription const& simDescription) : Model(simDescription) {
    continuousStates_ = State(1);
    continuousStates_[0] = 0.0; // initial output (y)

    derivative_ = std::make_shared<std::function<State(State const&, double)>>(
        [this](State const& y, double /*t*/) -> State {
            State dydt(1);
            dydt[0] = (gain_ * input_ - y[0]) / timeConstant_;
            return dydt;
        });

    jacobian_ = std::make_shared<std::function<Matrix(State const&, double)>>(
        [this](State const&, double) -> Matrix {
            Matrix jacobian(1, 1);
            jacobian(0, 0) = -1.0 / timeConstant_;
            return jacobian;
        });
}

void FirstOrderLag::update() {
    // No discrete states; nothing to update.
}

Derivative const& FirstOrderLag::derivative() const {
    return derivative_;
}

JacobianFunction const& FirstOrderLag::jacobian() const {
    return jacobian_;
}

Values FirstOrderLag::getOutputs(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "output") {
            values.push_back(type::core::Value::makeDouble(continuousStates_[0]));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void FirstOrderLag::setInputs(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "input") {
            input_ = value.asDouble();
        }
    }
}

Values FirstOrderLag::getInputs(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "input") {
            values.push_back(type::core::Value::makeDouble(input_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

Values FirstOrderLag::getParameters(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "timeConstant") {
            values.push_back(type::core::Value::makeDouble(timeConstant_));
        }
        else if (id.getName() == "gain") {
            values.push_back(type::core::Value::makeDouble(gain_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void FirstOrderLag::setParameters(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "timeConstant") {
            timeConstant_ = value.asDouble();
        }
        else if (id.getName() == "gain") {
            gain_ = value.asDouble();
        }
    }
}

FRELSIM_REGISTER_MODEL("FirstOrderLag", FirstOrderLag)

} // frelsim::model::algorithm
