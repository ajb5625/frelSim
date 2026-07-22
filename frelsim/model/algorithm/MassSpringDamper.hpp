#pragma once
#include "../core/Model.hpp"

namespace frelsim::model::algorithm {

/**
 * \file MassSpringDamper.hpp
 * \brief Standard second-order linear plant: m*x'' + c*x' + k*x = F(t).
 * A minimal continuous-only component intended as the thing a controller
 * (e.g. PIDController) drives via its "force" input.
 *
 * Continuous states: [position, velocity].
 * Input: "force" - external force F applied to the mass.
 * Outputs: "position", "velocity".
 */
class MassSpringDamper final : public core::Model {

    public:
        explicit MassSpringDamper(sim::proto::SimulationDescription const& simDescription);

        Values getOutputs(Identifiers ids) const override;

        void setInputs(SetOperations ops) override;

        Values getInputs(Identifiers ids) const override;

        Values getParameters(Identifiers ids) const override;

        void setParameters(SetOperations ops) override;

    protected:
        void update() override;

        Derivative const& derivative() const override;

    private:
        /// \brief Mass (kg).
        double mass_ = 1.0;

        /// \brief Damping coefficient (N*s/m).
        double damping_ = 0.5;

        /// \brief Spring constant (N/m).
        double springConstant_ = 1.0;

        /// \brief External force currently applied (N), from setInputs.
        double force_ = 0.0;

        Derivative derivative_;

};

} // frelsim::model::algorithm
