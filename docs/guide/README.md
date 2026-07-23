# frelsim user's guide

frelsim ("Federated Runtime for Engineering Loops") is a C++20 hybrid
(continuous + discrete) co-simulation framework: components with their own
internal dynamics are composed into one system, wired together, and stepped
forward together. This guide covers how to *use* it. For *why* it's built
this way - design decisions, bugs found and fixed, things tried and
reverted - see [`../progress/`](../PROGRESS.md) instead.

## Quick start

```bash
make            # build libfrelsim.a
make sim        # build the sim executable runner
./build/frelsim_sim examples/pid_mass_spring_damper.json
# Simulation complete at t=0.5
```

That config drives a `MassSpringDamper` plant to a setpoint via a
`PIDController` - see it in full in
[`composition.md`](composition.md#wiring-an-edge).

## Sections

1. [**Architecture overview**](architecture.md) - the three-stage pipeline
   (`Compiler` → `Linker` → `Simulator`), the component model
   (`Model`/`Task`/`Scheduler`/`EventEngine`), and the type system, all in
   one place.
2. [**Models**](models.md) - every built-in model (inputs, outputs,
   parameters) and how to write your own.
3. [**Composing a System**](composition.md) - the config format: wiring
   components together, initial parameters, and the validation `Linker`
   performs (including two real gotchas worth knowing up front).
4. [**Running simulations**](running-simulations.md) - `Overseer`, the
   `frelsim_sim` executable, the run-to-completion vs. single-step control
   modes, and logging a time series to verify results.
5. [**Solvers**](solvers.md) - the fixed-step/variable-step hierarchy,
   available solver types, and automatic (Jacobian-driven) solver
   selection.
6. [**Building and testing**](building.md) - requirements, Makefile
   targets, and the self-registration linking gotcha.

## What's not built yet

gRPC services (`Simulation`, `Simulator`), an FMU adapter, and a REST layer
are all designed for (the protos exist; `Overseer`'s method names already
mirror `Simulator.proto`'s service one-to-one) but not implemented. Check
the project's live task list for current status.
