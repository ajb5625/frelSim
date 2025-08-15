#pragma once
#include <memory>
#include <functional>
#include <Eigen/Dense>


namespace frelsim {

using State = Eigen::VectorXd;

using Derivative = std::shared_ptr<std::function<State(const State&, double)>>;

using Matrix = Eigen::MatrixXd;

using JacobianFunction = std::shared_ptr<std::function<Matrix(const State&, double)>>;

constexpr double SolverTolerance = 1e-9;

} // frelsim::util