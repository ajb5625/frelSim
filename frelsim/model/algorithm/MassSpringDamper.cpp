#include "MassSpringDamper.hpp"
#include "../factory/ModelFactory.hpp"

namespace frelsim::model::algorithm {

MassSpringDamper::MassSpringDamper(sim::proto::SimulationDescription const& simDescription) : Model(simDescription) {
    continuousStates_ = State(2);
    continuousStates_[0] = 0.0; // initial position (m)
    continuousStates_[1] = 0.0; // initial velocity (m/s)

    derivative_ = std::make_shared<std::function<State(State const&, double)>>(
        [this](State const& y, double /*t*/) -> State {
            State dydt(2);
            double const position = y[0];
            double const velocity = y[1];
            dydt[0] = velocity;
            dydt[1] = (force_ - damping_ * velocity - springConstant_ * position) / mass_;
            return dydt;
        });
}

void MassSpringDamper::update() {
    // MassSpringDamper has no discrete states; nothing to update.
}

Derivative const& MassSpringDamper::derivative() const {
    return derivative_;
}

Values MassSpringDamper::getOutputs(Identifiers ids) const {
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

void MassSpringDamper::setInputs(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "force") {
            force_ = value.asDouble();
        }
    }
}

Values MassSpringDamper::getParameters(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "mass") {
            values.push_back(type::core::Value::makeDouble(mass_));
        }
        else if (id.getName() == "damping") {
            values.push_back(type::core::Value::makeDouble(damping_));
        }
        else if (id.getName() == "springConstant") {
            values.push_back(type::core::Value::makeDouble(springConstant_));
        }
        else {
            values.push_back(type::core::Value::makeDouble(0.0));
        }
    }
    return values;
}

void MassSpringDamper::setParameters(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "mass") {
            mass_ = value.asDouble();
        }
        else if (id.getName() == "damping") {
            damping_ = value.asDouble();
        }
        else if (id.getName() == "springConstant") {
            springConstant_ = value.asDouble();
        }
    }
}

FRELSIM_REGISTER_MODEL("MassSpringDamper", MassSpringDamper)

} // frelsim::model::algorithm
