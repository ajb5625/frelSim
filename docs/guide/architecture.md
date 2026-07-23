# Architecture overview

frelsim ("Federated Runtime for Engineering Loops") is a C++20 hybrid
(continuous + discrete) co-simulation framework: components with their own
internal dynamics (continuous ODEs, discrete/periodic control logic, or
both) are composed into one system, wired together, and stepped forward in
lock-step by a central orchestrator.

## The three-stage pipeline

Every simulation goes through three stages, each owned by its own
component:

```
   JSON config file           System proto            LinkedSystem              running
        │                          │                       │                  Simulations
        ▼                          ▼                       ▼                       ▼
   ┌──────────┐   parse     ┌──────────┐   construct  ┌──────────┐   step()  ┌──────────┐
   │ Compiler │ ──────────► │  (data)  │ ────+────►   │  Linker  │ ────────► │Simulator │
   └──────────┘             └──────────┘  validate     └──────────┘           └──────────┘
```

1. **`Compiler`** (stage 1, structural): turns a JSON config file into a
   structural `System` proto. Doesn't know anything about composition
   semantics - just "is this well-formed JSON matching `System`'s shape."
2. **`Linker`** (stage 2, wiring): constructs every composed `Simulation`
   from the `System`, applies each component's `initial_parameters`, and
   validates every wired edge - unknown references, and a real
   type-check (not just hoping a mismatch throws somewhere) between a
   source's output type and a destination's input type. Produces a
   `LinkedSystem`.
3. **`Simulator`** (stage 3, execution): takes an already-linked,
   already-validated system and does exactly one thing well: advance time.
   No construction, no validation - by the time `Simulator` exists, all of
   that already happened.

**`Overseer`** is the single entry point that owns invoking all three
stages and exposes the two ways to run a simulation (see
[`running-simulations.md`](running-simulations.md)). It's what the
`frelsim_sim` executable, and eventually a gRPC/REST layer, sit on top of.

`Overseer` also optionally wires a **`Recorder`** (`frelsim/recorder/`)
into `Simulator`'s step loop, if a config's `logged_outputs` is non-empty -
this is how you verify a simulation's actual trajectory rather than only
inspecting a final snapshot; see
[`running-simulations.md`](running-simulations.md#verifying-results-logging-a-time-series).

## The component model: `Model`, `Task`/`Scheduler`, `EventEngine`

A **`Model`** is one hybrid dynamical system: it can hold continuous state
(integrated by a `Solver`), discrete state (updated on a schedule by a
`Task`/`Scheduler`), or zero-crossing events (detected by an `EventEngine`),
or any combination. See [`models.md`](models.md) for the built-in catalog
and how to write a new one.

- **`Task`/`Scheduler`** decide *when* a `Model`'s discrete `update()` should
  fire - continuous (every step), periodic (a fixed period + offset), or
  aperiodic (a one-shot time). `Model::guaranteeUntil(maxTime)` asks "how
  far can I safely advance before I'd miss something" (a scheduled
  update, or a zero-crossing event) - this is what lets `Simulator` compute
  a safe step size across every composed component at once.
- **`EventEngine`** detects zero-crossings (e.g. `BouncingBall`'s
  ground-contact bounce) via a lookahead search over an `EventIndicator`
  function, without needing to actually re-integrate the model at every
  candidate time.
- **`Solver`** integrates continuous state forward. See
  [`solvers.md`](solvers.md) for the fixed-step vs. variable-step
  distinction and automatic solver selection.

## Composition: `Simulation`, `System`, `Simulator`

A **`Simulation`** is one composed unit - it wraps a `SimAdapter`, which in
turn wraps either a `Model` (via `ModelAdapter`) or, eventually, an FMU or
handwritten-code target reached over gRPC. This is the abstraction that
lets frelsim compose heterogeneous simulation targets uniformly.

A **`System`** (the proto message) describes a whole composition: a list of
`RoutedSimulation` entries, each naming one component to construct plus a
`source`/`destinations` wiring edge. See
[`composition.md`](composition.md) for the format and its gotchas.

**`Simulator`** steps every composed `Simulation` forward together: each
step, it computes the minimum safe horizon across all of them
(conservative lock-step), routes values along every wired edge
(Jacobi-style - reads each source's *previous* step's value, not solved to
a fixed point), then steps everyone to that common horizon.

## The type system

Values crossing any co-simulation boundary (a `Model`'s inputs/outputs/
parameters, wire messages) are `frelsim::type::core::Value` - raw bytes
plus a `Type` descriptor, not a tree of tagged sub-values. This is
deliberate: FMUs and handwritten-code components over gRPC want a raw
buffer with a known layout, not something to walk. `Layout`/`TypeRegistry`
compute C-struct-style offsets/alignment (explicit little-endian, safe
across machines), and a struct/array schema can be registered once under a
URI and referenced by `type_ref` instead of repeated inline description.

See [`../progress/type-system.md`](../progress/type-system.md) for the
full design rationale.

## Where to go next

- [`models.md`](models.md) - the built-in models, and how to write your own
- [`composition.md`](composition.md) - wiring components into a `System`
- [`running-simulations.md`](running-simulations.md) - actually running one
- [`solvers.md`](solvers.md) - the numerical solver hierarchy
- [`building.md`](building.md) - build/test instructions
