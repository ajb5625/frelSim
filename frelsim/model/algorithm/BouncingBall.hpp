#pragma once
#include "../core/Model.hpp"

namespace frelsim::model::algorithm {

/**
 * \file BouncingBall.hpp
 * \brief Classic hybrid-system example: a ball in free fall that bounces
 * off the ground with energy loss given by a restitution coefficient.
 *
 * Continuous states: [height, velocity].
 * Event: zero crossing on height (Falling), which flips and scales velocity.
 */
class BouncingBall final : public core::Model {

    public:
        explicit BouncingBall(sim::proto::SimulationDescription const& simDescription);

        Values getOutputs(Identifiers ids) const override;

        void setInputs(SetOperations ops) override;

        Values getParameters(Identifiers ids) const override;

        void setParameters(SetOperations ops) override;

    protected:
        void update() override;

        Derivative const& derivative() const override;

        std::vector<event::Event> const& events() const override;

    private:
        /// \brief Acceleration due to gravity (m/s^2).
        double gravity_ = 9.81;

        /// \brief Fraction of velocity retained after a bounce.
        double restitution_ = 0.8;

        Derivative derivative_;

        std::vector<event::Event> events_;

};

} // frelsim::model::algorithm
