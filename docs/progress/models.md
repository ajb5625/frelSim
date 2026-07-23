# Models (`frelsim/model/`)

`frelsim/model/core/` (`Model` base class), `frelsim/model/adapt/`
(`ModelAdapter`), `frelsim/model/factory/` (`ModelFactory`), and
`frelsim/model/algorithm/` (the concrete models: `BouncingBall`,
`PIDController`, `MassSpringDamper`, `FirstOrderLag`, `BangBangController`,
`LongitudinalVehicle`).

## `Model`'s public/protected split

`getOutputs`/`setInputs`/`getInputs`/`getParameters`/`setParameters` are
public (the co-sim I/O boundary, called externally by `ModelAdapter`);
`update`/`derivative`/`jacobian`/`events` are protected (subclass
customization points, called internally by `Model`'s own non-virtual
`stepUntil`). State vectors (`continuousStates_`, `discreteStates_`,
`inputs_`, `outputs_`, `parameters_`) are protected so subclasses can
read/write them directly - a `Model` subclass typically also keeps its own
named domain members (e.g. `gravity_`, `kp_`) rather than routing everything
through the generic vectors.

`Model::stepUntil`'s bootstrap sentinel: `internalTime_` starts at `-1.0`,
meaning "nothing has happened yet." The very first `stepUntil` call on any
fresh `Model` is therefore always a **zero-length bootstrap step** (from an
indeterminate "before time" to whatever time it's first called with) - real
integration only happens from the second call onward. This trips up anyone
calling a `Model` directly (bypassing `Simulator`) for the first time: see
each model's test file for the `stepUntil(0.0); stepUntil(realTarget);`
two-call pattern this requires.

## First real model: `BouncingBall`

Free-fall dynamics (`derivative()`) plus a ground-contact zero-crossing
event (`events()`) using `EventEngine` (see
[`scheduling-and-events.md`](scheduling-and-events.md) for the bugs this
surfaced there). Result: parabolic free fall, bounce velocity flips sign
and decays by the restitution coefficient (~0.8x) each impact, apex height
decreasing bounce over bounce as expected.

Also surfaced, independent of `EventEngine`:
- A pure virtual `SimAdapter` destructor declared but never defined (link
  error the moment any subclass is destroyed).
- `RungeKutta4` declared an out-of-line destructor but never defined it
  (missing vtable key function - link error).
- `almostEqual` was a non-`inline` free function defined directly in a
  header, included by many translation units - an ODR violation invisible
  to `ar` (which doesn't check for duplicate symbols across object files)
  and only caught at final link time.

## `ModelFactory` self-registration

Before `MassSpringDamper`/`PIDController` were added, `ModelFactory.cpp` had
to `#include` every concrete model just to build an if/else dispatch chain -
adding a model meant editing a file that had no business knowing about it.
Switched to a self-registering factory: each model registers itself (name ->
constructor closure) via a static initializer in its own `.cpp`
(`FRELSIM_REGISTER_MODEL` macro in `ModelFactory.hpp`), so `ModelFactory`
never references any concrete model at all. See
[`build-and-tooling.md`](build-and-tooling.md) for the `--whole-archive`
linking requirement this pattern introduces.

## `MassSpringDamper` and `PIDController`

Added together to prove out a real control loop (see
[`simulator.md`](simulator.md) for the orchestration side). Both follow
`BouncingBall`'s pattern: own their domain concepts as plain private members
(gains/mass/etc.), not the generic `parameters_`/`discreteStates_` vectors.

- **`MassSpringDamper`**: `m*x'' + c*x' + k*x = F`, continuous, states
  `[position, velocity]`, `force` input.
- **`PIDController`**: discrete, periodically-sampled - no continuous state
  at all; `update()` runs the whole `error/integral/derivative/output` law
  on its Task's schedule, which must be `PeriodicDiscrete` for `update()` to
  ever fire. `measurement` input, `output` output, `kp`/`ki`/`kd`/`setpoint`
  parameters.

## `Model::getInputs` (added for real wiring type-checks)

Originally, `Model` only exposed `getOutputs`/`getParameters` - there was no
way to even ask a model "what's your current value for this input", and
`ModelAdapter::get` silently dropped `"Input"`-scoped queries entirely. This
mattered once `Linker` needed to type-check a wired edge (see
[`pipeline.md`](pipeline.md)): there was nothing to compare a source's
output type against on the destination side. Added
`Model::getInputs(Identifiers) const` (default empty, same convention as
`getParameters`), wired `ModelAdapter::get` to dispatch `"Input"` scope to
it, and implemented it in every model that actually takes an input
(`PIDController`'s `measurement`, `MassSpringDamper`'s `force`, and later
`FirstOrderLag`'s `input`, `BangBangController`'s `measurement`,
`LongitudinalVehicle`'s `throttle`). A model with no inputs (`BouncingBall`)
just keeps the default - and, per `Linker`'s design, can't legally be wired
as a destination as a result (there'd be nothing to type-check against).

## More control-system models (task #13)

Three more models, building toward a vehicle, following the same
established pattern.

**`FirstOrderLag`** - `tau*y' + y = K*u`, the simplest possible plant with
genuinely different dynamics than `MassSpringDamper` (first-order, no
oscillation) - a common building block (e.g. approximating actuator/sensor
lag) and a second, qualitatively different plant for test breadth. Supplies
a Jacobian (`dy'/dy = -1/tau`) since it's essentially free to compute
exactly. Tested against the closed-form step response
`y(t) = K*u*(1 - e^(-t/tau))`.

**`BangBangController`** - a relay (on/off) controller: `output = +amplitude`
if `error >= 0` else `-amplitude`. The simplest nontrivial alternative to
`PIDController` - discrete, no continuous state, but a genuinely
discontinuous control law rather than PID's smooth one, which is exactly
the point (test breadth across qualitatively different control laws, not
just more PID variants).

**`LongitudinalVehicle`** - the actual step toward a vehicle, deliberately
scoped to longitudinal-only dynamics (no steering/lateral motion yet):
```
mass * v' = throttle - dragCoefficient*v*|v| - rollingResistance*v
x' = v
```
The quadratic drag term is written as `v*|v|` rather than `sign(v)*v^2` so
it (and its derivative) stay smooth through `v=0` - no branch, no event
needed the way `BouncingBall`'s bounce needs one for its own sign flip. This
is also the first model in this codebase with a genuinely nonlinear
derivative (`MassSpringDamper`'s is linear), making it a real exercise for
`SolverType::Automatic`'s Jacobian-based stiffness detection (see
[`solvers.md`](solvers.md)) rather than only synthetic cases.

Testing note: `derivative()`/`jacobian()` are protected on `Model` by
design (subclass customization points, not part of the public co-sim I/O
boundary), so the Jacobian couldn't be unit-tested directly via a
finite-difference comparison without breaking that encapsulation - the
first attempt at this test referenced accessors that don't (and shouldn't)
exist. Verified indirectly but genuinely instead: an equilibrium test drives
the vehicle with constant throttle to its analytic terminal velocity
(`dragCoefficient*v^2 + rollingResistance*v - throttle = 0`, solved via the
quadratic formula) through the default adaptive solver, and the same check
repeated while forcing `SolverType::BackwardEuler` - which uses the
Jacobian for its Newton-Raphson iteration every step, so a wrong Jacobian
(wrong sign, wrong factor on the `|v|` term) would show up as convergence
to the wrong state rather than passing by coincidence.

Full suite as of this track: 129/129 passing.
