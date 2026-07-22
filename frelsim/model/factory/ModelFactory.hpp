#pragma once
#include <functional>
#include <memory>
#include <string>
#include "../core/Model.hpp"
#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::model::factory {

using ModelCreator = std::function<std::unique_ptr<core::Model>(sim::proto::SimulationDescription const&)>;

/**
 * \brief Registers `creator` under `componentName`, so a later createModel()
 * call with that component_name dispatches to it. Not normally called
 * directly - see FRELSIM_REGISTER_MODEL below, which each Model subclass
 * uses to register itself from its own .cpp, so ModelFactory never needs to
 * #include (or otherwise know about) any concrete Model.
 * \returns true, so it can be used as a static initializer's value.
 */
bool registerModel(std::string const& componentName, ModelCreator creator);

/**
 * \brief Constructs the Model registered under
 * simDescription.model_spec().component_name(), or nullptr if no model with
 * that name was registered.
 */
std::unique_ptr<core::Model> createModel(sim::proto::SimulationDescription const& simDescription);

} // frelsim::model::factory

/**
 * \brief Place at namespace scope (inside the class's own namespace) in a
 * Model subclass's .cpp to register it under `name`, e.g.:
 *   FRELSIM_REGISTER_MODEL("BouncingBall", BouncingBall)
 *
 * This relies on the registration running as a static initializer when the
 * subclass's .o is linked in. Since libfrelsim is a static archive (.a) and
 * nothing else references these .o files once ModelFactory itself doesn't
 * #include them, the linker would otherwise drop them entirely (an archive
 * member is only pulled in to resolve an outstanding undefined symbol) -
 * the Makefile links libfrelsim with --whole-archive specifically so this
 * works; see the LDFLAGS comment there before changing how libfrelsim.a is
 * linked into an executable.
 */
#define FRELSIM_REGISTER_MODEL(name, ClassName) \
    namespace { \
        const bool ClassName##_registered_ = ::frelsim::model::factory::registerModel( \
            name, [](::frelsim::sim::proto::SimulationDescription const& simDescription) { \
                return std::make_unique<ClassName>(simDescription); \
            }); \
    }
