# Solvers

Set via `model_spec.solver_type` in a component's `SimulationDescription`
(see [`composition.md`](composition.md)).

## Available solver types

| `SolverType` | Family | Notes |
|---|---|---|
| `Euler` | Fixed-step | First-order, simplest |
| `RungeKutta4` | Fixed-step | Classic 4th-order |
| `BackwardEuler` | Fixed-step, implicit | Newton-Raphson each step; needs a `jacobian()` from the model |
| `DormandPrince` | Variable-step | Embedded RK5(4) - the same method behind MATLAB's `ode45`/SciPy's `RK45`; adapts its own step size via local error control |
| `Automatic` | - | Runs a one-time stiffness diagnostic on the model's Jacobian at its initial state, then resolves to `BackwardEuler` (stiff) or `DormandPrince` (not stiff) |

`Euler`/`RungeKutta4`/`BackwardEuler` use the component's task
period/`max_step_size` as their fixed step size.  `DormandPrince`/
`Automatic` additionally read `model_spec.relative_tolerance`/
`absolute_tolerance` (optional; sensible defaults - 1e-6 relative, 1e-9
absolute - if unset).

## `SolverType::Automatic`

Requires the model to supply a `jacobian()` (see
[`models.md`](models.md#writing-a-new-model)) - without one, it falls back
to `DormandPrince` unconditionally, since stiffness can't be assessed (or
handled implicitly) without a Jacobian.

This is a **one-time choice at initialization**, not a dynamic
mid-simulation switch - nothing watches for a system becoming stiff partway
through a run. If your model's stiffness genuinely varies a lot over the
course of a simulation, pick a solver explicitly instead of relying on this.

The stiffness diagnostic itself (`frelsim::integrate::analysis::
assessStiffness`) uses the ratio of the Jacobian's fastest- to
slowest-decaying stable-mode eigenvalues - see
[`../progress/solvers.md`](../progress/solvers.md) for the full rationale
and the 1000x rule-of-thumb threshold.

## Writing a new solver

Decide which family it belongs to first:

- **Fixed-step**: subclass `frelsim::integrate::core::FixedStepSolver` and
  implement `singleStep(state, currentTime, dt)` - one step of exactly
  `dt`. The base class handles sub-stepping to reach whatever target
  `step()` is asked to reach.
- **Variable-step**: subclass `frelsim::integrate::core::VariableStepSolver`
  and implement `trialStep(state, currentTime, dt, errorEstimate)` - one
  attempt at a step of size `dt`, returning a candidate new state plus a
  local error estimate. The base class owns the accept/reject/resize loop.

Then register it in your solver's own `.cpp`:

```cpp
FRELSIM_REGISTER_SOLVER(::frelsim::sim::proto::SolverType::YourType,
    [](double stopTime, ::frelsim::integrate::factory::SolverConfig const& config,
       ::frelsim::Derivative const& f, ::frelsim::JacobianFunction const& jf) {
        return std::make_unique<YourSolverClass>(/* ... */);
    });
```

The creator is a full lambda (not a generic macro-generated shape) because
fixed-step and variable-step solvers genuinely take different constructor
arguments - see `Euler.cpp`, `DormandPrince45.cpp`, and `BackwardEuler.cpp`
for the three shapes already in use.
