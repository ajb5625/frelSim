#include "StiffnessDetector.hpp"
#include <Eigen/Eigenvalues>
#include <algorithm>
#include <limits>

namespace frelsim::integrate::analysis {

StiffnessAssessment assessStiffness(Matrix const& jacobian, double threshold) {
    if (jacobian.rows() == 0) {
        return {false, 0.0};
    }

    // General (non-symmetric) eigenvalue solver - a Jacobian from an
    // arbitrary nonlinear system has no reason to be symmetric.
    // computeEigenvectors=false since only the eigenvalues are needed here.
    Eigen::EigenSolver<Matrix> solver(jacobian, /*computeEigenvectors=*/false);
    auto const& eigenvalues = solver.eigenvalues();

    double slowest = std::numeric_limits<double>::infinity();
    double fastest = 0.0;
    bool foundStableMode = false;

    for (Eigen::Index i = 0; i < eigenvalues.size(); ++i) {
        double const realPart = eigenvalues[i].real();
        // Only stable (decaying) modes impose the explicit step-size limit
        // stiffness is about; marginal/unstable modes (realPart >= 0) don't
        // and are skipped rather than folded into the ratio.
        if (realPart >= 0.0) {
            continue;
        }
        double const magnitude = std::abs(realPart);
        slowest = std::min(slowest, magnitude);
        fastest = std::max(fastest, magnitude);
        foundStableMode = true;
    }

    if (!foundStableMode || slowest <= 0.0) {
        return {false, 0.0};
    }

    double const ratio = fastest / slowest;
    return {ratio >= threshold, ratio};
}

} // namespace frelsim::integrate::analysis
