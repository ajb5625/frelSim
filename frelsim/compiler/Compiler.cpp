#include "Compiler.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <google/protobuf/util/json_util.h>

namespace frelsim::compiler {

sim::proto::System Compiler::compile(std::string const& configPath) const {
    std::ifstream file(configPath);
    if (!file) {
        throw std::invalid_argument("Compiler: could not open config file '" + configPath + "'");
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    sim::proto::System system;
    google::protobuf::util::Status const status =
        google::protobuf::util::JsonStringToMessage(buffer.str(), &system);
    if (!status.ok()) {
        throw std::invalid_argument(
            "Compiler: failed to parse '" + configPath + "' as a System: " + status.ToString());
    }

    return system;
}

} // namespace frelsim::compiler
