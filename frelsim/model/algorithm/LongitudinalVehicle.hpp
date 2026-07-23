#pragma once
#include "../core/Model.hpp"

namespace frelsim::model::algorithm {

/**
 * \file LongitudinalVehicle.hpp
 * \brief A simple point-mass vehicle, longitudinal dynamics only (no
 * steering/lateral motion - that's future work once this is wired up and
 * proven out). Deliberately the first, simplest step toward a vehicle
 * rather than a full multi-DOF model: one throttle input, two continuous
 * states, two opposing-force terms.
 *
 * mass * v' = throttle - dragCoefficient*v*|v| - rollingResistance*v
 * x' = v
 *
 * The quadratic drag term is written as v*|v| rather than sign(v)*v^2 so it
 * (and its derivative) stay smooth through v=0 - no branch, no event needed
 * to handle a sign change the way BouncingBall needs one for its bounce.
 * This is also the first model in this codebase with a genuinely nonlinear
 * derivative (MassSpringDamper's is linear), making it a real exercise for
 * SolverType::Automatic's Jacobian-based stiffness detection rather than
 * only the synthetic cases in StiffnessDetectorTest.
 *
 * Continuous states: [position, velocity].
 * Input: "throttle" - net driving force (N); negative values brake.
 * Outputs: "position", "velocity".
 * Parameters: "mass" (kg), "dragCoefficient" (N*s^2/m^2), "rollingResistance" (N*s/m).
 */
class LongitudinalVehicle final : public core::Model {

    public:
        explicit LongitudinalVehicle(sim::proto::SimulationDescription const& simDescription);

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
        /// \brief Vehicle mass (kg).
        double mass_ = 1500.0;

        /// \brief Quadratic aerodynamic drag coefficient (N*s^2/m^2).
        double dragCoefficient_ = 0.35;

        /// \brief Linear rolling resistance coefficient (N*s/m).
        double rollingResistance_ = 10.0;

        /// \brief Current commanded driving force (N), from setInputs.
        double throttle_ = 0.0;

        Derivative derivative_;

        JacobianFunction jacobian_;

};

} // frelsim::model::algorithm
