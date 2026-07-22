#include <gtest/gtest.h>
#include "frelsim/integrate/analysis/StiffnessDetector.hpp"

namespace frelsim::integrate::analysis {
namespace {

TEST(StiffnessDetectorTest, ClassifiesWidelySeparatedDecayRatesAsStiff) {
    // Classic textbook stiff system: one mode decaying at rate 1, another
    // at rate 1000 - ratio 1000 sits right at the default threshold.
    Matrix jacobian(2, 2);
    jacobian << -1.0, 0.0,
                 0.0, -1000.0;

    auto const assessment = assessStiffness(jacobian);

    EXPECT_TRUE(assessment.isStiff);
    EXPECT_NEAR(assessment.stiffnessRatio, 1000.0, 1e-9);
}

TEST(StiffnessDetectorTest, ClassifiesComparableDecayRatesAsNotStiff) {
    // Decay rates within a small factor of each other - no reason to force
    // an implicit solver.
    Matrix jacobian(2, 2);
    jacobian << -1.0, 0.0,
                 0.0, -2.0;

    auto const assessment = assessStiffness(jacobian);

    EXPECT_FALSE(assessment.isStiff);
    EXPECT_NEAR(assessment.stiffnessRatio, 2.0, 1e-9);
}

TEST(StiffnessDetectorTest, IgnoresUnstableAndMarginalModesWhenFormingTheRatio) {
    // One genuinely fast stable mode, plus an unstable mode and a purely
    // oscillatory (zero real part) mode that must not be folded into the
    // ratio - only the negative-real-part modes bound an explicit solver's
    // step size the way stiffness is actually about.
    Matrix jacobian(3, 3);
    jacobian << -1000.0,  0.0, 0.0,
                    0.0,  5.0, 0.0,
                    0.0,  0.0, 0.0;

    auto const assessment = assessStiffness(jacobian);

    // Only one stable mode exists (-1000), so fastest == slowest == 1000
    // and the ratio is 1 - not stiff, despite the large-magnitude
    // eigenvalue, since there's nothing slower to compare it against.
    EXPECT_FALSE(assessment.isStiff);
    EXPECT_NEAR(assessment.stiffnessRatio, 1.0, 1e-9);
}

TEST(StiffnessDetectorTest, ReportsNotStiffWhenThereAreNoStableModes) {
    Matrix jacobian(1, 1);
    jacobian << 5.0; // purely unstable

    auto const assessment = assessStiffness(jacobian);

    EXPECT_FALSE(assessment.isStiff);
    EXPECT_EQ(assessment.stiffnessRatio, 0.0);
}

TEST(StiffnessDetectorTest, ReportsNotStiffForAnEmptyJacobian) {
    Matrix const jacobian(0, 0);

    auto const assessment = assessStiffness(jacobian);

    EXPECT_FALSE(assessment.isStiff);
    EXPECT_EQ(assessment.stiffnessRatio, 0.0);
}

TEST(StiffnessDetectorTest, RespectsACustomThreshold) {
    Matrix jacobian(2, 2);
    jacobian << -1.0, 0.0,
                 0.0, -50.0;

    // Ratio 50 doesn't clear the default threshold (1000)...
    EXPECT_FALSE(assessStiffness(jacobian).isStiff);
    // ...but does clear a caller-supplied lower one.
    EXPECT_TRUE(assessStiffness(jacobian, /*threshold=*/10.0).isStiff);
}

} // namespace
} // namespace frelsim::integrate::analysis
