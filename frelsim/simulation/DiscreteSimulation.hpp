// #pragma once

// #include "Simulation.hpp"

// namespace frelsim::simulation {

// class DiscreteSimulation : public Simulation {
//     public:
//         DiscreteSimulation(double sampleTime);

//         virtual ~DiscreteSimulation() override = default;

//         virtual void stepUntil(double stopTime) override;

//         SimulationType type() override;

//         virtual void update();
    
//     private:
//         double sampleTime_;

// };

// } // frelsim::simulation