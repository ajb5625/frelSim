#include "EventEngine.hpp"
#include "../util/almost_equal.hpp"

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

    double tL = t0;
    std::vector<double> yL = evaluateIndicators(tL, continuousStates, discreteStates, inputs);

    while (tL < maxTime) {
        const double h  = std::min(maxSearchStepSize_, maxTime - tL);
        double tR = tL + h;
        std::vector<double> yR = evaluateIndicators(tR, continuousStates, discreteStates, inputs);

        struct Candidate { size_t idx; double time; };
        std::vector<Candidate> cands;
        cands.reserve(events_.size());

        for (std::size_t idx = 0; idx < events_.size(); idx++) {
            auto& event = events_[idx];

            if (!std::isfinite(yL[idx]) || !std::isfinite(yR[idx])) {
                cands[idx] = {idx, tL};
                continue;
            }
            if (event.firesBetween(yL[idx], yR[idx])) {
                const double tRoot = bisectRoot(idx, tL, tR, continuousStates, discreteStates, inputs);
                cands[idx] = {idx, tRoot};
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
    while (pendingTime_.has_value()) {
        for (size_t idx : pendingEventIndices_) {
            events_[idx].handleEventAt(currentTime, continuousStates, discreteStates, params, inputs, outputs);
            ran = true;
        }

        clearPending();
        auto inst = zerosAt(currentTime, continuousStates, discreteStates, inputs);
        if (!inst.empty()) {
            pendingTime_ = currentTime;
            pendingEventIndices_ = std::move(inst);
        }
    }
    return ran;
}

void EventEngine::clearPending() {
    pendingTime_.reset();
    pendingEventIndices_.clear();
}

std::vector<double> EventEngine::evaluateIndicators(double time
                                        , State const& continuousStates
                                        , State const& discreteStates
                                        , Values const& inputs) const
{
    std::vector<double> vals;
    vals.reserve(events_.size());
    for (const auto& event : events_) {
        vals.push_back(event.evaluateIndicatorAt(time, continuousStates, discreteStates, inputs));
    }
    return vals;
}

double EventEngine::evaluateOneIndicator(size_t idx, double time,
                State const& continuousStates,
                State const& discreteStates,
                Values const& inputs) const {
    return events_[idx].evaluateIndicatorAt(time, continuousStates, discreteStates, inputs);
}

double EventEngine::bisectRoot(size_t idx, double a, double b,
                    State const& continuousStates,
                    State const& discreteStates,
                    Values const& inputs) const {
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

std::vector<size_t> EventEngine::zerosAt(double time,
                            State const& continuousStates,
                            State const& discreteStates,
                            Values const& inputs) const
{
    std::vector<size_t> idxs;
    if (events_.empty()) {
        return idxs;
    } 
    std::vector<double> g = evaluateIndicators(time, continuousStates, discreteStates, inputs);
    for (size_t i = 0; i < g.size(); ++i) {
        if (std::abs(g[i]) <= tolerance_) {
            idxs.push_back(i); 
        }
    }
    return idxs;
}

} // frelsim::event