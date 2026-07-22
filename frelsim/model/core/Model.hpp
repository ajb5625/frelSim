#pragma once
#include <memory>
#include "../../integrate/factory/SolverFactory.hpp"
#include "../../schedule/Scheduler.hpp"
#include "../../event/Event.hpp"
#include "../../event/EventEngine.hpp"
#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::model::core {

/**
 * \file Model.hpp
 * \brief Model is the smallest piece of a frelsim target.
 * It is a hybrid system containing potentially both discrete and 
 * continuous states.
 * A Simulation adapts into a Model using the ModelAdapter.
 */


// using Parameter = std::pair<std::string, SimValue>;
// using Parameters = std::vector<Parameter>;

class Model {
    public:
        Model(const sim::proto::SimulationDescription& simDescription);

        virtual ~Model();

        void initialize();

        /**
         * \brief Give the latest possible time before
         * an event occurs. Guarantee until is non const because
         * it will preload events in the event engine which fire next.
         */
        virtual double guaranteeUntil(double maxTime);

        /**
         * \brief stepUntil is not virtual. This is on purpose.
         * We allow algorithm authors to create their own update
         * function, but we don't want them editing the content of the step itself.
         * The step is responsible for reading inputs, integrating continuous states,
         * updating discrete states, and writing outputs.
         */
        bool stepUntil(double stopTime);

        /**
         * \brief Given a list of identifiers, get the outputs associated with them.
         */
        virtual Values getOutputs(Identifiers ids) const = 0;

        /**
         * \brief Given a list of SetOperation (identifier and value), set the
         * values on the model.
         */
        virtual void setInputs(SetOperations ops) = 0;

        /**
         * \brief Given a list of identifiers, get the model's current value
         * for each named input - e.g. so Linker can compare an input's type
         * against a candidate source's output type before wiring them
         * together (see Linker::link()). Default is empty (matching
         * getParameters' default for models with none), since not every
         * model takes inputs (e.g. BouncingBall). A model that does take
         * inputs and wants its wiring to be type-checked should override
         * this the same way it overrides getOutputs.
         */
        virtual Values getInputs(Identifiers ids) const;

        virtual Values getParameters(Identifiers ids) const;

        virtual void setParameters(SetOperations ops);

    protected:
        /**
         * \brief Update discrete states.
         */
        virtual void update() = 0;

        /**
         * \brief Provide derivative of continuous states with respect to time.
         */
        virtual Derivative const& derivative() const = 0;

        /**
         * \brief Provide jacobian matrix-valued function of continuous states.
         * The jacobian can help the model determine which solver to use and
         * allows for more types of solvers to be used.
         */
        virtual JacobianFunction const& jacobian() const;

        /**
         * \brief Define the zero crossing events in your model.
         */
        virtual std::vector<event::Event> const& events() const;

        /// \brief Continuous states vector to be integrated each step.
        State continuousStates_;

        /// \brief Discrete states vector to be updated each step.
        State discreteStates_;

        Values inputs_;

        Values outputs_;

        Parameters parameters_;
        // std::map<std::string, SimValue> parameters_;

    private:
        /// \brief The solver for integrating continuous states.
        std::unique_ptr<integrate::core::Solver> solver_;

        /// \brief Metadata and setup info for the simulation.
        sim::proto::SimulationDescription simDescription_;

        std::unique_ptr<schedule::Scheduler> scheduler_;

        std::unique_ptr<event::EventEngine> eventEngine_;

        double internalTime_ = -1.0;

        double stepSize_;

        double stopTime_;

        double maxStepSize_;

};
} // frelsim::model::core
        
        
