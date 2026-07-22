#pragma once

#include <string>
#include "frelsim/proto/System.pb.h"

namespace frelsim::compiler {

/**
 * \file Compiler.hpp
 * \brief Stage 1 of the config -> link -> execute pipeline (see Overseer):
 * turns a JSON config file into a structural System proto. Purely
 * structural - "is this well-formed JSON that deserializes into a System" -
 * with no knowledge of composition semantics (unknown simulation
 * references, type mismatches between wired components, etc.); that
 * validation is Linker's job, one stage later, since it requires actually
 * constructing the composed Simulations to check.
 */
class Compiler {
    public:
        /**
         * \brief Reads configPath's contents and parses them as JSON into a
         * System proto (via google::protobuf::util::JsonStringToMessage).
         * \throws std::invalid_argument if the file can't be read, or its
         * contents aren't JSON that conforms to the System message shape.
         */
        sim::proto::System compile(std::string const& configPath) const;
};

} // namespace frelsim::compiler
