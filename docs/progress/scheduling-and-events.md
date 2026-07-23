# Scheduling, events, and utilities (`frelsim/task/`, `frelsim/schedule/`, `frelsim/event/`, `frelsim/util/`)

## `EventEngine` bugs found bringing up `BouncingBall` (MVP track)

Running the first real model end-to-end (not just compiling it) surfaced
several bugs that had never been caught because no executable had ever
actually linked against `libfrelsim.a` before that point:

- `EventEngine::nextEventTime` (the lookahead search) evaluated indicators
  against a single static, pre-step state snapshot while only varying the
  query time - so an indicator like "current height" could never observe a
  crossing, since it returned the same frozen value at every probe. Fixed by
  threading *elapsed time since the anchor state* through consistently
  everywhere indicators are evaluated, so an indicator can give a closed-form
  prediction (e.g. free-fall kinematics) instead of reading a frozen
  snapshot. This is now a load-bearing convention: an `EventIndicator`'s time
  argument is *elapsed time since the anchor state was captured*, not the
  absolute simulation clock. Post-step confirmation (`processEventsAt`)
  passes `elapsed=0` since the state there is already valid at that instant.
- A `std::vector::reserve()` + `operator[]` bug in the candidate list
  (`cands[idx] = ...`) wrote out of bounds and never actually grew the
  vector's size, so a detected crossing would never register as pending even
  once the indicator itself was fixed.
- A cascading-events infinite loop: a handler that intentionally settles its
  own indicator to exactly zero (e.g. a bounce handler snapping height to
  `0.0` for a resting contact) would immediately be seen as "still crossing"
  by the post-step cascade check and re-fire itself forever. Fixed by
  excluding events handled in the current round from that same round's
  re-check, plus a defensive round cap so a genuinely pathological model
  cascades a bounded number of times instead of hanging the simulation.

Regression tests: `EventEngineTest.LookaheadFindsCrossingUsingElapsedTimeNotAbsoluteTime`
(would fail against the pre-fix code - the old lookahead search could never
detect a crossing at all) and
`EventEngineTest.ProcessEventsAtDoesNotHangWhenHandlerParksIndicatorAtZero`
(would hang forever against the pre-fix code).

## The `offset=0` periodic-task scheduling bug (orchestration track)

A real, subtle bug found running the PID+`MassSpringDamper` control loop
demo (see [`simulator.md`](simulator.md)): the control loop initially ran but
never terminated - `position`/`control` kept evolving (looked like real
physics) while `Simulator`'s own clock stayed pinned at `t=0` for hundreds
of thousands of calls, and the PID output blew up to over 100 almost
immediately.

Root cause: `PIDController`'s task has `offset=0`, so right after `Model`'s
`-1.0` bootstrap step, `internalTime_` sits at exactly `0.0` - which is
*also* that task's own first scheduled boundary. `Model::guaranteeUntil`
asked `Scheduler::getNextDiscreteTime(internalTime_)` unnudged, which
correctly-by-its-own-rules reports "next firing: now" forever, since
nothing marks that boundary as already consumed - simulated time never
advanced past it, and `update()` (with its integral accumulation) re-fired
on every single orchestrator call instead of once per 0.05s.

Fixed with a one-line nudge (query `internalTime_ + TinyTolerance` instead
of `internalTime_` bare) scoped specifically to `guaranteeUntil` -
`stepUntil`'s own discrete-hit check deliberately keeps the inclusive,
unnudged query (it's asking "did the time I just stepped *to* land exactly
on a scheduled hit", a different question with different correct
semantics). `BouncingBall` never hit this because it has no
periodic/aperiodic task at all, and `EventEngine` was already immune via
`firesBetween`'s strict inequalities - this was specifically a
`PeriodicDiscrete`-task-with-`offset=0` gap.

Regression test: `PIDControllerTest.GuaranteeUntilAdvancesPastZeroOffsetTaskBoundary`
- would time out / never advance past `t=0` against the pre-fix code.

## `Identifier` (`frelsim/util/`)

Bug found while first writing tests for it: the old implementation wrote
out of bounds on a malformed URI instead of rejecting it, for a
`domain.scope.name` string with too many `.`-separated parts. Fixed
alongside writing `IdentifierTest.RejectsUriWithTooManyParts`, which would
have failed (or worse, silently corrupted memory) against the pre-fix code.
