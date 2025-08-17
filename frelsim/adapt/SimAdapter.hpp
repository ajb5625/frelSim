#pragma once

#include "../util/Aliases.hpp"

namespace frelsim::adapt {

/**
 * \file Adapter.hpp
 * \brief The SimAdapter tells a Simulation how to call step, get, and set.
 * This is an interface only class intended to be overridden for frelsim, FMU, and Code.
 */

class SimAdapter {
    public:
        SimAdapter() = default;

        virtual ~SimAdapter() = 0; 

        virtual bool stepUntil(double stopTime) = 0;

        virtual Values get(Identifiers ids) const = 0;

        virtual void set(SetOperations ops) = 0;

};
} // frelsim::adapt