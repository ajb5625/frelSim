#include "EventEngine.hpp"
#include "../util/almost_equal.hpp"
#include <algorithm>

namespace frelsim::event {

EventEngine::EventEngine(std::vector<Event>const& events,
                    double tolerance,
                    double maxSearchStep) : events_(events)
                                            , tolerance_(tolerance)
                                            , maxSearchStepSize_(maxSearchStep) {}


double EventEngine::nextEventTime(double t0
                    , double maxTime
                    , State const& continuousStates
                    , State const& discreteStates
                    , Values const& inputs) {
    clearPending();
    if (events_.empty() || maxTime <= t0) {
        return maxTime;
    }

    // continuousStates/discreteStates are a single snapshot taken at t0 - we do
    // NOT re-integrate the model as we scan forward (that would mean re-running
    // the solver at every probe point, which defeats the purpose of a cheap
    // lookahead). So every indicator evaluation below happens against that same
    // fixed anchor state, and the only thing that varies from probe to probe is
    // how far past t0 we're asking the indicator to predict. We always pass
    // "elapsed time since t0" into the indicator (never the absolute simulation
    // time) so an indicator can answer with a closed-form prediction, e.g. a
    // free-falling body's height is h0 + v0*elapsed - 0.5*g*elapsed^2.
    double tL = t0;
    std::vector<double> yL = evaluateIndicators(tL - t0, continuousStates, discreteStates, inputs);

    while (tL < maxTime) {
        const double h  = std::min(maxSearchStepSize_, maxTime - tL);
        double tR = tL + h;
        std::vector<double> yR = evaluateIndicators(tR - t0, continuousStates, discreteStates, inputs);

        struct Candidate { size_t idx; double time; };
        std::vector<Candidate> cands;
        cands.reserve(events_.size());

        for (std::size_t idx = 0; idx < events_.size(); idx++) {
            auto& event = events_[idx];

            if (!std::isfinite(yL[idx]) || !std::isfinite(yR[idx])) {
                // reserve() only preallocates capacity, it does not grow size(),
                // so cands[idx] here would write out of bounds - push_back is
                // the correct way to append a candidate.
                cands.push_back({idx, tL});
                continue;
            }
            if (event.firesBetween(yL[idx], yR[idx])) {
                // bisectRoot works in the same elapsed-since-t0 space as the
                // indicator evaluations above, so convert this window's bounds
                // to elapsed time before calling it, then convert the elapsed
                // root it finds back to an absolute simulation time for cands.
                const double elapsedRoot = bisectRoot(idx, tL - t0, tR - t0, continuousStates, discreteStates, inputs);
                cands.push_back({idx, t0 + elapsedRoot});
            }
        }

        if (!cands.empty()) {
            const double tMin = std::min_element(
                cands.begin(), cands.end(),
                [](const Candidate& a, const Candidate& b){
                    return a.time < b.time;
                }
            )->time;

            pendingTime_ = tMin;
            pendingEventIndices_.clear();
            for (const auto& c : cands) {
                if (util::almostEqual(c.time, tMin)) {
                    pendingEventIndices_.push_back(c.idx);
                }
            }
            return *pendingTime_;
        }
        tL = tR;
        yL.swap(yR);
    }
    return maxTime;
}

bool EventEngine::processEventsAt(double currentTime
                    , State& continuousStates
                    , State& discreteStates
                    , Parameters& params
                    , Values const& inputs
                    , Values& outputs) {
    if (!pendingTime_.has_value()) {
        return false;
    }
    bool ran = false;
    // A handler very often resolves its own event by parking the indicator
    // exactly at zero (e.g. a bounce handler snapping height to exactly 0.0 for
    // a resting/contact state). zerosAt() below only looks at indicator
    // magnitude, so without tracking which events we JUST handled this round,
    // that same event would be seen as "still at zero" and re-fired forever.
    // We only want genuine cascades here - a DIFFERENT event whose indicator
    // happens to also be zero as a side effect of this round's handlers - so
    // each round's own just-handled indices are excluded from that same
    // round's re-check.
    std::vector<size_t> justHandled;

    // A malformed model could still cascade forever (e.g. two events whose
    // handlers keep tripping each other), so cap the number of rounds rather
    // than let a single time step hang the whole simulation.
    constexpr int maxCascadeRounds = 100;
    int round = 0;

    while (pendingTime_.has_value() && round < maxCascadeRounds) {
        ++round;
        for (size_t idx : pendingEventIndices_) {
            events_[idx].handleEventAt(currentTime, continuousStates, discreteStates, params, inputs, outputs);
            ran = true;
        }
        justHandled = pendingEventIndices_;

        clearPending();
        // By this point continuousStates/discreteStates are the model's real,
        // already-integrated state at currentTime (the solver has run and the
        // handlers above have applied), not a lookahead anchor - so there is no
        // "elapsed" to speak of here, the indicator should just be asked about
        // right now, i.e. elapsed = 0.
        auto inst = zerosAt(continuousStates, discreteStates, inputs);

        std::vector<size_t> cascaded;
        for (size_t idx : inst) {
            bool const justHandledThisIdx = std::find(justHandled.begin(), justHandled.end(), idx) != justHandled.end();
            if (!justHandledThisIdx) {
                cascaded.push_back(idx);
            }
        }

        if (!cascaded.empty()) {
            pendingTime_ = currentTime;
            pendingEventIndices_ = std::move(cascaded);
        }
    }
    return ran;
}

void EventEngine::clearPending() {
    pendingTime_.reset();
    pendingEventIndices_.clear();
}

std::vector<double> EventEngine::evaluateIndicators(double elapsed
                                        , State const& continuousStates
                                        , State const& discreteStates
                                        , Values const& inputs) const
{
    std::vector<double> vals;
    vals.reserve(events_.size());
    for (const auto& event : events_) {
        vals.push_back(event.evaluateIndicatorAt(elapsed, continuousStates, discreteStates, inputs));
    }
    return vals;
}

double EventEngine::evaluateOneIndicator(size_t idx, double elapsed,
                State const& continuousStates,
                State const& discreteStates,
                Values const& inputs) const {
    return events_[idx].evaluateIndicatorAt(elapsed, continuousStates, discreteStates, inputs);
}

double EventEngine::bisectRoot(size_t idx, double a, double b,
                    State const& continuousStates,
                    State const& discreteStates,
                    Values const& inputs) const {
    // a and b are elapsed-since-anchor bounds (see evaluateOneIndicator above),
    // not absolute simulation times; the caller is responsible for converting
    // back to an absolute time once this returns.
    double ya = evaluateOneIndicator(idx, a, continuousStates, discreteStates, inputs);
    double yb = evaluateOneIndicator(idx, b, continuousStates, discreteStates, inputs);
    if (!std::isfinite(ya) || !std::isfinite(yb)) {
        return a;
    }
    if (ya == 0.0) {
        return a;
    }
    if (yb == 0.0) {
        return b;
    }
    const int maxIters = 64;
    for (int it = 0; it < maxIters; ++it) {
        const double m  = 0.5 * (a + b);
        const double ym = evaluateOneIndicator(idx, m, continuousStates, discreteStates, inputs);

        if (!std::isfinite(ym)) {
            return a;
        }
        const bool sameSign = (ya <= 0.0 && ym <= 0.0) || (ya >= 0.0 && ym >= 0.0);
        if (sameSign) {
             a = m; ya = ym;
        }
        else {
            b = m; yb = ym;
        }
        const double width = std::abs(b - a);
        const double scale = 1.0 + std::max(std::abs(a), std::abs(b));
        if (width <= tolerance_ * scale) {
            return a;
        }
    }
    return a;
}

std::vector<size_t> EventEngine::zerosAt(State const& continuousStates,
                            State const& discreteStates,
                            Values const& inputs) const
{
    std::vector<size_t> idxs;
    if (events_.empty()) {
        return idxs;
    }
    // elapsed = 0: continuousStates/discreteStates are the model's real current
    // state, so we're asking "is the indicator at zero right now", not
    // predicting forward from some earlier anchor.
    std::vector<double> g = evaluateIndicators(0.0, continuousStates, discreteStates, inputs);
    for (size_t i = 0; i < g.size(); ++i) {
        if (std::abs(g[i]) <= tolerance_) {
            idxs.push_back(i);
        }
    }
    return idxs;
}

} // frelsim::event
