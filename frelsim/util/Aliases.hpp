#pragma once
#include <memory>
#include <functional>
#include <Eigen/Dense>
#include "Identifier.hpp"


namespace frelsim {

using State = Eigen::VectorXd;

using Derivative = std::shared_ptr<std::function<State(const State&, double)>>;

using Matrix = Eigen::MatrixXd;

using JacobianFunction = std::shared_ptr<std::function<Matrix(const State&, double)>>;

constexpr double SolverTolerance = 1e-9;

constexpr double TinyTolerance = 1e-12;

using SimValue = double;

using Values = std::vector<SimValue>;

using Parameters = std::vector<SimValue>;

using Identifiers = std::vector<util::Identifier>;

using SetOperations = std::vector<std::pair<util::Identifier, SimValue>>;

} // frelsim::util