#pragma once
#include <memory>
#include <functional>
#include <Eigen/Dense>
#include "Identifier.hpp"
#include "../type/core/Value.hpp"


namespace frelsim {

using State = Eigen::VectorXd;

using Derivative = std::shared_ptr<std::function<State(const State&, double)>>;

using Matrix = Eigen::MatrixXd;

using JacobianFunction = std::shared_ptr<std::function<Matrix(const State&, double)>>;

constexpr double SolverTolerance = 1e-9;

constexpr double TinyTolerance = 1e-12;

// Named, typed I/O values (getOutputs/setInputs/getParameters/setParameters,
// SimAdapter::get/set) - not to be confused with State above, which is the
// plain double vector a Solver integrates and stays untyped on purpose.
using SimValue = type::core::Value;

using Values = std::vector<SimValue>;

using Parameters = std::vector<SimValue>;

using Identifiers = std::vector<util::Identifier>;

using SetOperations = std::vector<std::pair<util::Identifier, SimValue>>;

} // frelsim::util