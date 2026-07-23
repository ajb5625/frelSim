#pragma once
#include "../core/Model.hpp"

namespace frelsim::model::algorithm {

/**
 * \file FirstOrderLag.hpp
 * \brief Classic first-order lag: tau*y' + y = K*u, i.e. y' = (K*u - y)/tau.
 * A minimal continuous plant with different dynamics than MassSpringDamper
 * (first-order, no oscillation) - useful as a second simple plant for test
 * breadth, and a common building block (e.g. approximating actuator or
 * sensor lag) alongside a more physical plant like LongitudinalVehicle.
 *
 * Continuous state: [output].
 * Input: "input" - u.
 * Output: "output" - y.
 * Parameters: "timeConstant" (tau, seconds), "gain" (K).
 */
class FirstOrderLag final : public core::Model {

    public:
        explicit FirstOrderLag(sim::proto::SimulationDescription const& simDescription);

        Values getOutputs(Identifiers ids) const override;

        void setInputs(SetOperations ops) override;

        Values getInputs(Identifiers ids) const override;

        Values getParameters(Identifiers ids) const override;

        void setParameters(SetOperations ops) override;

    protected:
        void update() override;

        Derivative const& derivative() const override;

        JacobianFunction const& jacobian() const override;

    private:
        /// \brief Time constant (s). Must stay positive - see the derivative.
        double timeConstant_ = 1.0;

        /// \brief Steady-state gain.
        double gain_ = 1.0;

        /// \brief Current input u, from setInputs.
        double input_ = 0.0;

        Derivative derivative_;

        JacobianFunction jacobian_;

};

} // frelsim::model::algorithm
