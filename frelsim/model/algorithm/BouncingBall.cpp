#include "BouncingBall.hpp"

namespace frelsim::model::algorithm {

BouncingBall::BouncingBall(sim::proto::SimulationDescription const& simDescription) : Model(simDescription) {
    continuousStates_ = State(2);
    continuousStates_[0] = 10.0; // initial height (m)
    continuousStates_[1] = 0.0;  // initial velocity (m/s)

    derivative_ = std::make_shared<std::function<State(State const&, double)>>(
        [this](State const& y, double /*t*/) -> State {
            State dydt(2);
            dydt[0] = y[1];
            dydt[1] = -gravity_;
            return dydt;
        });

    // EventEngine's lookahead search evaluates this indicator many times against
    // a single fixed (continuousStates, discreteStates) snapshot without ever
    // re-integrating the model in between - so `elapsed` here is time SINCE that
    // snapshot was taken, not the absolute simulation clock (see the comment on
    // EventIndicator in Event.hpp). We predict height analytically from the
    // anchor height/velocity using free-fall kinematics: h(elapsed) = h0 + v0*elapsed
    // - 0.5*g*elapsed^2. At elapsed == 0 (the post-step confirmation call from
    // EventEngine::processEventsAt) this correctly reduces to just h0, i.e. the
    // real current height.
    event::EventIndicator groundIndicator = [this](double elapsed
                                              , State const& continuousStates
                                              , State const& /*discreteStates*/
                                              , Values const& /*inputs*/) -> double {
        double const h0 = continuousStates[0];
        double const v0 = continuousStates[1];
        return h0 + v0 * elapsed - 0.5 * gravity_ * elapsed * elapsed;
    };

    event::EventHandler bounceHandler = [this](double /*t*/
                                              , State& continuousStates
                                              , State& /*discreteStates*/
                                              , Parameters& /*params*/
                                              , Values const& /*inputs*/
                                              , Values& /*outputs*/) {
        continuousStates[0] = 0.0;
        continuousStates[1] = -restitution_ * continuousStates[1];
    };

    events_.emplace_back(groundIndicator, bounceHandler, event::EventType::Falling);
}

void BouncingBall::update() {
    // BouncingBall has no discrete states; nothing to update.
}

Derivative const& BouncingBall::derivative() const {
    return derivative_;
}

std::vector<event::Event> const& BouncingBall::events() const {
    return events_;
}

Values BouncingBall::getOutputs(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "height") {
            values.push_back(continuousStates_[0]);
        }
        else if (id.getName() == "velocity") {
            values.push_back(continuousStates_[1]);
        }
        else {
            values.push_back(0.0);
        }
    }
    return values;
}

void BouncingBall::setInputs(SetOperations ops) {
    (void)ops; // BouncingBall is autonomous; it has no external inputs.
}

Values BouncingBall::getParameters(Identifiers ids) const {
    Values values;
    values.reserve(ids.size());
    for (auto const& id : ids) {
        if (id.getName() == "gravity") {
            values.push_back(gravity_);
        }
        else if (id.getName() == "restitution") {
            values.push_back(restitution_);
        }
        else {
            values.push_back(0.0);
        }
    }
    return values;
}

void BouncingBall::setParameters(SetOperations ops) {
    for (auto const& [id, value] : ops) {
        if (id.getName() == "gravity") {
            gravity_ = value;
        }
        else if (id.getName() == "restitution") {
            restitution_ = value;
        }
    }
}

} // frelsim::model::algorithm
