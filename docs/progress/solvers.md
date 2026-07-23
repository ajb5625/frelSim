# Numerical solvers (`frelsim/integrate/`)

`frelsim/integrate/core/` (`Solver`, `FixedStepSolver`, `VariableStepSolver`),
`frelsim/integrate/expl/` (`Euler`, `RungeKutta4`, `DormandPrince45`),
`frelsim/integrate/impl/` (`BackwardEuler`), `frelsim/integrate/factory/`
(`SolverFactory`), `frelsim/integrate/analysis/` (`StiffnessDetector`).

## The fixed step-size bug

First attempt: change `Solver::step` to take the actual elapsed `dt` per
call (computed by `Model::stepUntil` as `stopTime - internalTime_`) instead
of blindly using a step size fixed at construction. This fixed a real
correctness bug (see below for how it was found), but on review missed the
point of a *fixed-step* solver - it shifted all responsibility for choosing
a sensible, accuracy-appropriate step size onto the caller, with nothing
left enforcing it. Euler/RK4/BackwardEuler are supposed to own their own
step size for accuracy and stability, not just integrate whatever gap a
caller happens to ask for.

**How it was found**: running the PID+`MassSpringDamper` control loop
demo (see [`simulator.md`](simulator.md)) with `Simulator` taking irregular
step sizes (the composition mixes `MassSpringDamper`'s 0.02s grid with
PID's 0.05s grid, so real gaps between calls are sometimes 0.02s and
sometimes 0.01s) - `Solver::step()` always integrated by its own fixed
constructed `stepSize_` regardless of the actual gap between calls, so the
0.01s-gap calls still integrated a full 0.02s of dynamics. Convergence was
qualitatively correct (stable, converges toward the setpoint) but not
numerically precise while step sizes stayed irregular.

**Settled design**: the standard fixed-step contract. `Solver` keeps its own
`stepSize_` (from the component's task period/`max_step_size`), and
`step()` takes a *target* time rather than a `dt` - it internally sub-steps
in increments of at most `stepSize_` until it reaches that target, however
many sub-steps that takes. This still fixes the original bug (a 0.01s gap
and a 0.02s gap are each covered by the right amount of integration, not the
same fixed amount regardless), while restoring the solver's own accuracy
guarantee regardless of what a caller asks for.

Implementation: the sub-stepping loop is identical across all three
fixed-step solvers, so it's factored once into the base class, calling a
protected pure-virtual `singleStep(state, currentTime, dt)` that each
concrete solver implements. Also fixed a second, previously-invisible bug
found while touching this code: the derivative function `f_` was being
evaluated at the *target* time instead of the *start* time of each step
(invisible for every model built so far, since none of their dynamics
explicitly depend on `t` - but wrong in general, and `BackwardEuler`'s
implicit residual/Jacobian specifically need the *end*-of-step time,
`currentTime + dt`, a different value again from either).

Verified against the PID + `MassSpringDamper` demo: still terminates
correctly at `stop_time`, and the trajectory is measurably different from
(more accurate than) the pre-fix run, confirming the fix changes real
numerical behavior rather than just internal plumbing.

## Solver hierarchy split + variable-step (RK45) solver

The fixed-step contract above (owns a `stepSize_`, sub-steps internally) is
genuinely only one of two fundamentally different solver shapes - a
variable-step solver adapts its own step size call to call based on local
error control, and forcing both shapes through one `Solver` interface would
mean either bolting adaptive-step fields onto a fixed-step solver or vice
versa. Split `Solver` (now genuinely bare - just
`f_`/`jacobianFunction_`/`stopTime_` and the `step()` contract) into:

- **`FixedStepSolver`** - exactly what `Solver` held before this split (the
  sub-stepping loop, `stepSize_`). `Euler`/`RungeKutta4`/`BackwardEuler`
  derive from this instead of `Solver` directly.
- **`VariableStepSolver`** - new. Owns the generic accept/reject/step-size-
  adaptation loop (mixed relative+absolute scaled-RMS error norm, standard
  practice for embedded Runge-Kutta pairs - the same shape MATLAB's `ode45`
  or SciPy's `RK45` use), computed via Eigen array expressions rather than a
  manual index loop. Concrete solvers implement only `trialStep()` - one
  attempt at a step of a given size, returning a candidate new state plus a
  local error estimate; the base decides accept/reject and how to resize
  the next attempt. Same "base owns the loop, subclass owns the one
  numerical operation" shape as `FixedStepSolver`/`Model::stepUntil`.

**`DormandPrince45`**: the embedded RK5(4) method (`DormandPrince` had sat
as a `SolverType` enum value returning `nullptr` from `SolverFactory` since
the very first version of this project) - `trialStep()` runs all 7 stages
of the standard Dormand-Prince Butcher tableau and returns the 5th-order
solution, with the error estimate computed directly as the (5th-order
weights - 4th-order weights) combination rather than assembling both
solutions separately. Tests specifically include a time-varying derivative
(`dy/dt = t`) rather than only the exponential-decay benchmark used
elsewhere - decay's derivative ignores its time argument entirely, so it
can't catch a transcription error in the tableau's time-node coefficients
(c2..c6) the way a genuinely time-dependent case can. Also a call-counting
test confirming a looser tolerance actually takes fewer derivative
evaluations than a tighter one, to demonstrate the step-size adaptation is
really engaging and not just accepting every step at some fixed size.

`SolverFactory::createSolver` takes a `SolverConfig` aggregate instead of a
single `stepSize` parameter, since fixed-step and variable-step solvers
need different tuning knobs (the config carries both; each solver family
uses only the subset it needs). `ModelSpec` gained optional
`relative_tolerance`/`absolute_tolerance` fields so a config can tune
`DormandPrince45`'s accuracy per-component; unset means sensible built-in
defaults (tighter than most: 1e-6 relative, 1e-9 absolute).

## `SolverFactory` self-registration

`SolverFactory.cpp` used to `#include` every concrete solver header and
switch over `SolverType` to construct one - the same coupling
`ModelFactory` had before it moved to self-registration (see
[`models.md`](models.md)). Fixed the same way: `SolverFactory` now exposes
`registerSolver(SolverType, SolverCreator)` backed by a
`map<SolverType, SolverCreator>`, and each solver (`Euler`, `RungeKutta4`,
`DormandPrince45`, `BackwardEuler`) registers itself from its own `.cpp` via
a new `FRELSIM_REGISTER_SOLVER` macro - `SolverFactory.hpp/.cpp` no longer
`#include` any concrete solver at all.

One real difference from `FRELSIM_REGISTER_MODEL`: `ModelCreator` has one
uniform signature, but solver constructors genuinely differ by family -
fixed-step (`stopTime, stepSize, f[, jf]`) vs. variable-step
(`stopTime, relTol, absTol, minStep, maxStep, f`). Rather than force a
one-size macro over that, `FRELSIM_REGISTER_SOLVER` takes the creator lambda
itself as an argument (still built from the common `SolverConfig`), so each
solver's `.cpp` writes the one-line lambda matching its own constructor
shape; the macro only removes the registry-plumbing boilerplate. Same
`--whole-archive` linking requirement as `FRELSIM_REGISTER_MODEL` applies.

Added `SolverFactoryTest` - there was previously zero coverage of
`createSolver` itself (only direct solver construction was tested), which
would have silently missed a registration/linking regression of exactly the
kind this refactor could introduce.

## Jacobian-driven stiffness detection + automatic solver selection

Added `SolverType::Automatic` (a new additive enum value - existing values
and the proto3 default of 0/Euler are unchanged, so this is opt-in). When
requested, `SolverFactory::createSolver` runs a one-time diagnostic rather
than a direct registry lookup: it evaluates the model's Jacobian at its
initial state (time 0) and classifies stiffness, then resolves to a concrete
`SolverType` and proceeds exactly as it would if that type had been
requested directly. This is a single choice made once (diagnostic +
choose-once-at-`Model::initialize()`), not a dynamic mid-simulation switch -
there's no machinery watching for the system to become stiff mid-run.

`frelsim/integrate/analysis/StiffnessDetector.{hpp,cpp}` holds the
diagnostic itself, kept separate from `SolverFactory` since "is this system
stiff" is a reusable question independent of solver construction:
`assessStiffness(Matrix const& jacobian, double threshold = 1000.0)` uses
the classic ratio-of-eigenvalue-real-parts heuristic (`Eigen::EigenSolver`,
since a Jacobian from an arbitrary nonlinear system has no reason to be
symmetric) - the ratio between a system's fastest- and slowest-decaying
*stable* modes (negative real part; marginal/unstable modes are excluded
since they don't impose the explicit step-size restriction stiffness is
actually about). A ratio at or above the threshold (1000 is the textbook
rule-of-thumb default) is reported stiff. This is inherently a
single-sample approximation, not a stiffness proof - good enough to pick a
solver family once at startup, not a substitute for a model author's own
judgment.

`SolverFactory::createSolver` gained a trailing `initialState` parameter
(defaulted, so every existing call site is unaffected) that only matters
for `Automatic`: stiff resolves to `BackwardEuler` (the only implicit
solver this framework has), not stiff resolves to `DormandPrince` (the best
general-purpose adaptive explicit default). Without a Jacobian or an initial
state to evaluate it at, it falls back to `DormandPrince` unconditionally,
since stiffness can't be assessed - and can't be handled implicitly -
without one. `Model::initialize()` threads `continuousStates_` through to
`createSolver`; it's already set to the model's real initial condition by
the subclass's constructor, which always runs before `initialize()` is
called, so no ordering change was needed.

Added `StiffnessDetectorTest` (including the "one fast mode alone isn't
stiff without something slower to compare against" and "unstable/marginal
modes must not be folded into the ratio" edge cases) and extended
`SolverFactoryTest` to check `Automatic` actually resolves to the right
concrete solver via `dynamic_cast` - stiff picks `BackwardEuler`, not-stiff
picks `DormandPrince`, and both no-Jacobian and no-initial-state fall back
to `DormandPrince`.

`LongitudinalVehicle` (see [`models.md`](models.md)) later became the first
real, non-synthetic exercise of this - its quadratic-drag derivative is
genuinely nonlinear, unlike every earlier model's linear or absent Jacobian.
