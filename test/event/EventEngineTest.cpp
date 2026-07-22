#include <gtest/gtest.h>
#include "frelsim/event/EventEngine.hpp"

namespace frelsim::event {
namespace {

// These are regression tests for two bugs found while getting BouncingBall
// running end-to-end (see docs/PROGRESS.md): (1) the lookahead search
// couldn't detect a crossing at all because it reused a static state
// snapshot without threading elapsed time through to the indicator, and (2)
// a handler that intentionally settles its own indicator to exactly zero
// (e.g. a bounce resting at height 0) would immediately re-trigger itself
// forever in the cascading-events check.

// A free-falling body's height, predicted analytically from an anchor
// (h0, v0) at a given elapsed time - same shape as BouncingBall's real
// indicator, but standalone here so the test doesn't depend on the Model
// layer at all.
EventIndicator makeHeightIndicator(double gravity) {
    return [gravity](double elapsed, State const& continuousStates, State const&, Values const&) -> double {
        double const h0 = continuousStates[0];
        double const v0 = continuousStates[1];
        return h0 + v0 * elapsed - 0.5 * gravity * elapsed * elapsed;
    };
}

TEST(EventEngineTest, LookaheadFindsCrossingUsingElapsedTimeNotAbsoluteTime) {
    // Anchor: height 5, velocity -5 (falling), no gravity for a simple linear
    // crossing: h(elapsed) = 5 - 5*elapsed, crosses zero at elapsed = 1.0.
    EventIndicator indicator = makeHeightIndicator(/*gravity=*/0.0);
    EventHandler noopHandler = [](double, State&, State&, Parameters&, Values const&, Values&) {};
    std::vector<Event> events{Event(indicator, noopHandler, EventType::Falling)};

    EventEngine engine(events, /*tolerance=*/1e-9, /*maxSearchStep=*/0.1);

    State continuousStates(2);
    continuousStates[0] = 5.0;
    continuousStates[1] = -5.0;
    State discreteStates;
    Values inputs;

    double crossingTime = engine.nextEventTime(/*t0=*/0.0, /*maxTime=*/2.0, continuousStates, discreteStates, inputs);
    EXPECT_NEAR(crossingTime, 1.0, 1e-6);
}

TEST(EventEngineTest, LookaheadReturnsMaxTimeWhenNoCrossingInWindow) {
    // Anchor: height 5, velocity -1 - won't reach zero within maxTime=2.0.
    EventIndicator indicator = makeHeightIndicator(/*gravity=*/0.0);
    EventHandler noopHandler = [](double, State&, State&, Parameters&, Values const&, Values&) {};
    std::vector<Event> events{Event(indicator, noopHandler, EventType::Falling)};

    EventEngine engine(events, /*tolerance=*/1e-9, /*maxSearchStep=*/0.1);

    State continuousStates(2);
    continuousStates[0] = 5.0;
    continuousStates[1] = -1.0;
    State discreteStates;
    Values inputs;

    double result = engine.nextEventTime(0.0, 2.0, continuousStates, discreteStates, inputs);
    EXPECT_DOUBLE_EQ(result, 2.0);
}

TEST(EventEngineTest, ProcessEventsAtDoesNotHangWhenHandlerParksIndicatorAtZero) {
    // Linear fall (gravity=0, so h(elapsed) = h0 + v0*elapsed): height 1.0,
    // velocity -1.0, crosses zero at elapsed = 1.0.
    EventIndicator indicator = makeHeightIndicator(/*gravity=*/0.0);
    int handlerCallCount = 0;
    // Handler mimics BouncingBall's bounce: snaps height to exactly 0 and
    // flips velocity. Since the post-step check evaluates the indicator at
    // elapsed=0 (i.e. it just reads h0, which the handler just set to exactly
    // 0), a naive cascade re-check would see "still at zero" and re-fire this
    // same event forever.
    EventHandler bounceHandler = [&handlerCallCount](double, State& continuousStates, State&, Parameters&, Values const&, Values&) {
        continuousStates[0] = 0.0;
        continuousStates[1] = -continuousStates[1];
        ++handlerCallCount;
    };
    std::vector<Event> events{Event(indicator, bounceHandler, EventType::Falling)};
    EventEngine engine(events, /*tolerance=*/1e-9, /*maxSearchStep=*/0.1);

    State continuousStates(2);
    continuousStates[0] = 1.0;
    continuousStates[1] = -1.0;
    State discreteStates;
    Values inputs;
    Values outputs;
    Parameters params;

    // Correctly primes pendingTime_ = 1.0 via the same lookahead search
    // exercised in the tests above - this is not a hand-set fixture, it's
    // going through the real code path a Model would use.
    double crossingTime = engine.nextEventTime(0.0, 2.0, continuousStates, discreteStates, inputs);
    ASSERT_NEAR(crossingTime, 1.0, 1e-6);

    bool ran = engine.processEventsAt(crossingTime, continuousStates, discreteStates, params, inputs, outputs);

    // The important assertion is that this line is ever reached at all -
    // before the fix, this call would hang forever. With the fix, the
    // handler settling its own indicator to exactly zero must not cause it
    // to be treated as a new cascade of itself, so it fires exactly once.
    EXPECT_TRUE(ran);
    EXPECT_EQ(handlerCallCount, 1);
    EXPECT_DOUBLE_EQ(continuousStates[0], 0.0);
    EXPECT_DOUBLE_EQ(continuousStates[1], 1.0);
}

} // namespace
} // namespace frelsim::event
