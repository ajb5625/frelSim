# Models

A `Model` is one hybrid dynamical system - continuous state, discrete
state, zero-crossing events, or any combination. Every built-in model lives
in `frelsim/model/algorithm/` and self-registers under a name used in a
config's `model_spec.component_name` (see
[`composition.md`](composition.md)).

Every input/output/parameter identifier below needs the matching `scope` -
`"Output"` to read an output, `"Input"` to read or write an input,
`"Parameter"` to read or write a parameter. This is easy to get wrong: an
identifier with the wrong (or no) scope is silently ignored rather than
erroring - see [`composition.md`](composition.md#a-real-gotcha-scope).

## Plants (continuous)

| Model | States | Input | Outputs | Parameters | Notes |
|---|---|---|---|---|---|
| `BouncingBall` | height, velocity | *(none - autonomous)* | `height`, `velocity` | `gravity`, `restitution` | Free fall + a ground-contact bounce event |
| `MassSpringDamper` | position, velocity | `force` | `position`, `velocity` | `mass`, `damping`, `springConstant` | `m*x'' + c*x' + k*x = F`, linear |
| `FirstOrderLag` | output | `input` | `output` | `timeConstant`, `gain` | `tau*y' + y = K*u`, first-order, no oscillation |
| `LongitudinalVehicle` | position, velocity | `throttle` | `position`, `velocity` | `mass`, `dragCoefficient`, `rollingResistance` | `m*v' = throttle - Cd*v|v| - Cr*v` - the first genuinely nonlinear model in this codebase |

## Controllers (discrete)

| Model | Input | Output | Parameters | Notes |
|---|---|---|---|---|
| `PIDController` | `measurement` | `output` | `kp`, `ki`, `kd`, `setpoint` | Periodically sampled; needs a `PeriodicDiscrete` task to ever fire |
| `BangBangController` | `measurement` | `output` | `setpoint`, `amplitude` | Relay/on-off: `+amplitude` if `error >= 0` else `-amplitude` |

## Writing a new model

Subclass `frelsim::model::core::Model` (`frelsim/model/core/Model.hpp`).
The public/protected split matters:

- **Public** (the co-sim I/O boundary - what `ModelAdapter`/`Linker` call
  from outside): `getOutputs`, `setInputs`, `getInputs`, `getParameters`,
  `setParameters`.
- **Protected** (your customization points, called internally by `Model`'s
  own non-virtual `stepUntil`): `update()` (discrete state), `derivative()`
  (continuous state's ODE right-hand side), `jacobian()` (optional - see
  below), `events()` (optional zero-crossings).

A minimal continuous plant looks like `MassSpringDamper` or
`LongitudinalVehicle`: own your domain concepts as plain private members
(not the generic `parameters_`/`discreteStates_` vectors), set
`continuousStates_` in the constructor, and return a `Derivative` (a
`shared_ptr<function<State(State const&, double)>>`) from `derivative()`.
A discrete-only controller looks like `PIDController` or
`BangBangController`: `continuousStates_ = State(0)`, a trivial empty
`derivative()`, and the whole control law in `update()`.

**Implement `getInputs` if your model takes any input.** This is not
optional if you want your model wireable as a `Linker` destination -
`Linker` type-checks a wired edge by comparing the source's actual output
type against the destination's *current* input value (via `getInputs`), and
a model that doesn't override it (default is empty, matching
`getParameters`' own default) fails loudly rather than skipping the check.
See [`../progress/models.md`](../progress/models.md) for why this exists.

**Supply a `jacobian()` if you can compute one easily** (see
`FirstOrderLag`'s one-liner, or `LongitudinalVehicle`'s 2x2 analytic
Jacobian) - it costs little and unlocks `SolverType::Automatic`'s
stiffness-based solver selection (see [`solvers.md`](solvers.md)) for
anyone composing your model.

Finally, register it:

```cpp
FRELSIM_REGISTER_MODEL("YourModelName", YourModelClass)
```

placed at namespace scope in your model's own `.cpp` (see any existing
model for the exact placement). This is what lets `component_name:
"YourModelName"` in a config resolve to your class, without
`ModelFactory` ever needing to `#include` it.
