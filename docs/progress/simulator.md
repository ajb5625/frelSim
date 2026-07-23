# Simulator (`frelsim/simulator/`)

`Simulator`'s job is purely to find the minimum safe horizon across every
composed `Simulation`'s own `guaranteeUntil`, route values, and step
everyone to that common time. It does **not** construct or validate the
composition anymore - see [`pipeline.md`](pipeline.md) for why that moved
to `Linker`.

## Orchestration + control loop demo

**Goal**: get a real composed simulation (not one component driven directly)
running through `Simulator`, using it to prove out `PIDController` +
`MassSpringDamper` (see [`models.md`](models.md)) as a classic
step-response control loop.

Removed the `scheduler_` member entirely - it was never constructed
(a null-deref bug from the MVP track) and turns out not to be needed: each
`Model` already manages its own scheduling internally. Routing is
Jacobi-style as agreed: every step, each wired edge
(`RoutedSimulation.source -> .destinations`, both fully-qualified
`domain.scope.name` identifiers) is read from its source and applied to its
destination(s) *before* anyone steps, so every component sees the *previous*
step's values - one step of lag, not solved to a fixed point. Time
advancement is conservative lock-step: the minimum of every composed
`Simulation`'s own `guaranteeUntil` horizon, computed each step - so a
single `Simulator::step(stopTime)` call only advances by at most the
system's `max_step_size`, not straight to `stopTime` in one call
(`stopTime` there is a cap on that one step's horizon, not a jump target -
reaching a distant target takes repeated calls, which is exactly the
single-step external-control mode `Overseer::step()` exposes; see
[`pipeline.md`](pipeline.md)).

Also added `RoutedSimulation.initial_parameters` (`repeated SetOperation`) -
without it there was no way to configure a component's initial values (PID
gains, a setpoint, a plant's mass) through the `System` composition at all.
Originally applied once in `Simulator::initialize()`, right after
construction - now applied by `Linker::link()` instead (see
[`pipeline.md`](pipeline.md)).

**Composition validation - originally a deliberate stopgap here.** Whether
a wired composition even makes sense (do the referenced simulations/ports
exist, are the types compatible) was, for a long time, `Simulator`'s
problem by default: `Simulator::initialize()` did a one-time dry-run over
every wired edge so an unknown reference or type mismatch surfaced at
startup instead of deep in the run loop. This was always flagged as not
really `Simulator`'s job - fast, trusting runtime loop vs. one-time
validation are different concerns - and was eventually extracted into
`Linker`; see [`pipeline.md`](pipeline.md) for where that logic actually
lives now (and how the type-checking was later made real rather than
incidental).

**A real, subtle scheduling bug found running the demo** - see
[`scheduling-and-events.md`](scheduling-and-events.md) for the `offset=0`
periodic-task story; it's a `Task`/`Scheduler` bug, not really a
`Simulator` one, even though this is where it surfaced.

**Result.** The demo (PID driving `MassSpringDamper` to a setpoint of 5,
both wired through a real `System`/`Simulator` composition) runs end-to-end
and terminates correctly at `stop_time`, converging toward the setpoint
with the expected damped-oscillation shape. The remaining numerical-
precision gap noted at the time (fixed step sizes ignoring irregular call
gaps) is covered in [`solvers.md`](solvers.md).

`Simulator` itself currently has no dedicated test coverage that predates
the compile/link/execute pipeline work - see `SimulatorTest` under
[`pipeline.md`](pipeline.md) for where that was finally added.
