#pragma once
#include <memory>
#include "../../integrate/factory/SolverFactory.hpp"
#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::model::core {

/**
 * \file Model.hpp
 * \brief Model is the smallest piece of a frelsim target.
 * It is a hybrid system containing potentially both discrete and 
 * continuous states.
 * A Simulation adapts into a Model using the ModelAdapter.
 */


using Parameter = std::pair<std::string, SimValue>;
using Parameters = std::vector<Parameter>;

class Model {
    public:
        Model(const sim::proto::SimulationDescription& simDescription);

        virtual ~Model();

        void initialize();

        /**
         * \brief stepUntil is not virtual. This is on purpose.
         * We allow algorithm authors to create their own update
         * function, but we don't want them editing the content of the step itself.
         * The step is responsible for reading inputs, integrating continuous states,
         * updating discrete states, and writing outputs.
         */
        bool stepUntil(double stopTime);

        virtual Values getOutputs(Identifiers ids) const;

        virtual void setInputs(SetOperations ops);

        virtual Values getParameters(Identifiers ids) const;

        virtual void setParameters(SetOperations ops);

    protected:
        /**
         * \brief Update discrete states.
         */
        virtual void update();

        virtual const Derivative& derivative() const;

        virtual const JacobianFunction& jacobian() const;

    private:
        /**
         * \brief Integrate continuous states.
         */
        void integrate();

        /// \brief The solver for integrating continuous states.
        std::unique_ptr<integrate::core::Solver> solver_;

        /// \brief Metadata and setup info for the simulation.
        sim::proto::SimulationDescription simDescription_;

        /// \brief Continuous states vector to be integrated each step.
        State continuousStates_;

        /// \brief Discrete states vector to be updated each step.
        State discreteStates_;

        Values inputs_;

        Values outputs_;

        std::map<std::string, SimValue> parameters_;

        double stepSize_;

        double stopTime_;



};
} // frelsim::model::core
        
        
