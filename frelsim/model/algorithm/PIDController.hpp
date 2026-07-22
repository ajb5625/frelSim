#pragma once
#include "../core/Model.hpp"

namespace frelsim::model::algorithm {

/**
 * \file PIDController.hpp
 * \brief Classic discrete, periodically-sampled PID controller - no
 * continuous state at all; the whole control law runs in update(), which
 * Model::stepUntil calls on the schedule of this component's Task (which
 * must be PeriodicDiscrete - see Task.hpp - for update() to ever fire).
 *
 * error = setpoint - measurement
 * integral += error * period
 * derivative = (error - previousError) / period
 * output = kp*error + ki*integral + kd*derivative
 *
 * Input: "measurement" - the process variable being controlled.
 * Output: "output" - the control signal.
 * Parameters: "kp", "ki", "kd", "setpoint".
 */
class PIDController final : public core::Model {

    public:
        explicit PIDController(sim::proto::SimulationDescription const& simDescription);

        Values getOutputs(Identifiers ids) const override;

        void setInputs(SetOperations ops) override;

        Values getInputs(Identifiers ids) const override;

        Values getParameters(Identifiers ids) const override;

        void setParameters(SetOperations ops) override;

    protected:
        void update() override;

        Derivative const& derivative() const override;

    private:
        /// \brief Sample period (s), read once from this component's Task at
        /// construction - used for the integral/derivative terms.
        double period_ = 0.0;

        double kp_ = 1.0;
        double ki_ = 0.0;
        double kd_ = 0.0;

        double setpoint_ = 0.0;

        /// \brief Current process variable, from setInputs.
        double measurement_ = 0.0;

        /// \brief Accumulated integral of error.
        double integral_ = 0.0;

        /// \brief Error on the previous update(), for the derivative term.
        double previousError_ = 0.0;

        /// \brief Current control signal, written by update(), read by getOutputs.
        double output_ = 0.0;

        /// \brief Trivial (zero-length) derivative - PIDController has no
        /// continuous state, but Model::derivative() is pure virtual.
        Derivative derivative_;

};

} // frelsim::model::algorithm
