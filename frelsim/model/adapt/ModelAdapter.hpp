#pragma once
#include "../../adapt/SimAdapter.hpp"
#include "frelsim/proto/Simulation.pb.h"
#include "../factory/ModelFactory.hpp"
#include "../core/Model.hpp"

namespace frelsim::model::adapt {

class ModelAdapter : public frelsim::adapt::SimAdapter {

    public:
        ModelAdapter(sim::proto::SimulationDescription simDescription);

        ~ModelAdapter() override;

        bool stepUntil(double stopTime) override;

        Values get(Identifiers ids) const override;

        void set(SetOperations ops) override;

    private:
        /// \brief Metadata and setup info for the simulation.
        sim::proto::SimulationDescription simDescription_;

        /// \brief Pointer to the dynamic system
        std::unique_ptr<core::Model> instance_;

};
} // frelsim::simulation



    