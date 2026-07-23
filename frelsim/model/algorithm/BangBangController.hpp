#pragma once
#include "../core/Model.hpp"

namespace frelsim::model::algorithm {

/**
 * \file BangBangController.hpp
 * \brief Classic relay (on/off) controller - the simplest nontrivial
 * alternative to PIDController: no continuous state, discrete/periodically
 * sampled, and its control law is genuinely discontinuous (output snaps
 * between +amplitude and -amplitude based only on the sign of the error),
 * unlike PID's smooth proportional/integral/derivative law. Useful both as
 * a second basic controller and as test breadth for a qualitatively
 * different control law.
 *
 * error = setpoint - measurement
 * output = +amplitude if error >= 0, else -amplitude
 *
 * Input: "measurement" - the process variable being controlled.
 * Output: "output" - the control signal.
 * Parameters: "setpoint", "amplitude".
 */
class BangBangController final : public core::Model {

    public:
        explicit BangBangController(sim::proto::SimulationDescription const& simDescription);

        Values getOutputs(Identifiers ids) const override;

        void setInputs(SetOperations ops) override;

        Values getInputs(Identifiers ids) const override;

        Values getParameters(Identifiers ids) const override;

        void setParameters(SetOperations ops) override;

    protected:
        void update() override;

        Derivative const& derivative() const override;

    private:
        double setpoint_ = 0.0;

        /// \brief Output magnitude when the relay is in either state.
        double amplitude_ = 1.0;

        /// \brief Current process variable, from setInputs.
        double measurement_ = 0.0;

        /// \brief Current control signal, written by update(), read by getOutputs.
        double output_ = 0.0;

        /// \brief Trivial (zero-length) derivative - no continuous state,
        /// but Model::derivative() is pure virtual.
        Derivative derivative_;

};

} // frelsim::model::algorithm
