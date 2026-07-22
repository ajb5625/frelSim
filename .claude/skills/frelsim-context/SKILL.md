---
name: frelsim-context
description: Re-orients Claude on the frelSim C++ co-simulation framework project (a hybrid discrete/continuous simulation framework with gRPC/protobuf-based distributed co-simulation) after a context reset or compaction. Captures git/PR workflow rules, build gotchas, and key architecture decisions that are easy to get wrong or silently re-break if forgotten. Use this at the start of any non-trivial work session in the frelSim repo (/home/aj/frelSim) - especially after a new conversation starts, context was compacted, or when about to run git/make commands or touch Model/Solver/EventEngine/Simulator/type-system code - since acting on stale or absent context here has repeatedly caused real regressions (e.g. re-breaking the Solver step-size fix, or pushing straight to main).
---

# frelSim project context

frelSim ("Federated Runtime for Engineering Loops") is a C++20 hybrid
(continuous + discrete) co-simulation framework: `Task`/`Scheduler`/`Event`
drive discrete-time behavior, `Model` wraps hybrid systems with pluggable
numerical `Solver`s, and `Simulator`/`Simulation`/`System` compose multiple
components (FMU, handwritten-code-over-gRPC, or native `frelsim` `Model`s)
into one co-simulation via gRPC/protobuf.

**Read `docs/PROGRESS.md` before making any non-trivial architecture
decision.** It's the full chronological narrative - design decisions, why
they were made, bugs found and their root causes, things tried and reverted.
This skill only captures the load-bearing summary; PROGRESS.md has the
reasoning. Don't duplicate its content here or let this file's summary
override it if they ever disagree - PROGRESS.md is the source of truth.

## Git workflow - the one rule that matters most

**Never push directly to `main`.** Always: create a feature branch, commit,
push the branch, and hand the user the PR-creation URL GitHub prints after
the push (`https://github.com/ajb5625/frelSim/pull/new/<branch>`). Wait for
the user to merge themselves. This was corrected explicitly by the user
after two direct pushes to `main` - treat it as a hard rule, not a
default-that-can-slide, for every change in this repo unless the user
explicitly says otherwise for a specific change.

After the user says they've merged: `git checkout main && git pull origin
main`, then delete the local feature branch. Start the next branch from a
freshly-pulled `main`, not from an old checkout.

## Build & test

- `make` - builds `build/libfrelsim.a`. `make clean` first if switching
  branches with structural changes (header dependency tracking via `-MMD
  -MP` is in place, but a full clean is safest across a branch switch).
- `make test` - builds and runs the GoogleTest suite (`build/frelsim_tests`).
  GMock-dependent test files (name contains `Mock`) are silently skipped
  until `libgmock-dev` is installed (see Environment quirks below) - `make
  test` still works either way.
- Both `make` and `make` (proto codegen + full recompile) are slow (~1-2
  min) - run them via a backgroundable/long-timeout mechanism rather than
  assuming they finish within a short command timeout.
- A `make`/`make test` pipeline like `make 2>&1 | tail -N` reports the exit
  code of `tail`, not `make` - a failed build can look like it succeeded if
  you only check the shell exit code. Check the actual log output for
  errors, don't trust `$?` after a pipe.

**Critical linking gotcha**: `Model` subclasses self-register with
`ModelFactory` via a static initializer in their own `.cpp`
(`FRELSIM_REGISTER_MODEL` macro - see `ModelFactory.hpp`), specifically so
`ModelFactory` never has to `#include` every concrete model. Because
`libfrelsim` is a static archive (`.a`), the linker only pulls an archive
member in to resolve an outstanding undefined symbol - since nothing else
references a model's `.o` once `ModelFactory` doesn't, a plain `ar`-built
archive link would silently drop it and its registration would just never
run (`createModel("SomeModel")` returns `nullptr` at runtime, no error).
**Any new executable that needs the model registry populated (the sim
runner, ad-hoc test drivers, anything linking `libfrelsim.a`) must link it
with `-Wl,--whole-archive libfrelsim.a -Wl,--no-whole-archive`** - the
Makefile's `LINK_LIBFRELSIM` variable does this; use it (or match it
exactly) rather than linking `$(TARGET)`/`build/libfrelsim.a` directly.

## Key architecture decisions already made (don't relitigate without reading why first)

- **Type system** (`frelsim::type::core::Value`): bytes + a `Type`
  descriptor, not a tree of tagged sub-values - deliberately shaped for
  handing values across process boundaries to FMUs/handwritten-code
  components, which want a raw buffer with a known layout, not a tree to
  walk. `Layout`/`TypeRegistry` compute C-struct-style offsets/alignment
  (explicit little-endian, safe across machines) and let a struct/array
  schema be registered once under a URI and referenced via `type_ref`
  instead of repeated inline. Struct fields must be fixed-size for now (no
  string/variable-length nested in a struct).
- **`Model`'s public/protected split**: `getOutputs`/`setInputs`/
  `getParameters`/`setParameters` are public (the co-sim I/O boundary,
  called externally by `ModelAdapter`); `update`/`derivative`/`jacobian`/
  `events` are protected (subclass customization points, called internally
  by `Model`'s own non-virtual `stepUntil`). State vectors
  (`continuousStates_`, `discreteStates_`, `inputs_`, `outputs_`,
  `parameters_`) are protected so subclasses can read/write them directly -
  a `Model` subclass typically also keeps its own named domain members
  (e.g. `gravity_`, `kp_`) rather than routing everything through the
  generic vectors; see `BouncingBall`/`PIDController`/`MassSpringDamper`.
- **`EventEngine` indicator convention**: an `EventIndicator`'s time
  argument is *elapsed time since the anchor state was captured*, not the
  absolute simulation clock - required because the lookahead search
  (`nextEventTime`) evaluates an indicator many times against one fixed
  pre-step state snapshot without re-integrating (too expensive to do
  otherwise), so an indicator must predict analytically from
  (state-at-anchor, elapsed) rather than read a frozen "current" value.
  Post-step confirmation (`processEventsAt`) passes elapsed=0 since the
  state there is already valid at that instant.
- **`Solver`'s fixed-step contract**: `step()` takes a *target* time, not a
  raw `dt` - the solver owns its own `stepSize_` (from the component's task
  period/`max_step_size`) and sub-steps internally in increments of at most
  `stepSize_` to reach the target, however many sub-steps that takes. This
  was deliberately chosen over just passing an explicit `dt` through from
  the caller, specifically so the solver keeps its own accuracy/stability
  guarantee regardless of what a caller asks for - don't revert to a
  pass-a-`dt`-straight-through design without re-reading why in
  PROGRESS.md's Solver correctness track.
- **`Simulator`'s routing**: Jacobi-style - every step, each wired edge
  (`RoutedSimulation.source -> .destinations`, both fully-qualified
  `domain.scope.name` identifiers, `domain` = the target `Simulation`'s key
  in `idToSimulation_`) is read from its source and applied to its
  destination(s) *before* anyone steps, so every component sees the
  *previous* step's values (one step of lag, not solved to a fixed point).
  Time advancement is conservative lock-step: the minimum of every composed
  `Simulation`'s own `guaranteeUntil` horizon, computed each step.
- **Composition validation is a deliberate stopgap, not solved**:
  `Simulator::initialize()` does a one-time dry-run over wired edges to
  catch unknown references/type mismatches at startup. A real validator (a
  `Linker`/`Compiler`-shaped component, discussed but not yet built) needs
  `Model` to be able to declare its port types without being instantiated
  and queried - nothing does that today. Check the live task list (next
  section) before assuming this has been built.

## Environment quirks (this dev machine specifically)

- Multiple gRPC/protobuf installs exist (system apt packages vs. a
  separately-built newer one under `/home/aj/.local`). The Makefile pins
  `grpc_cpp_plugin` to `/usr/bin/grpc_cpp_plugin` explicitly and derives
  `pkg-config` search paths from wherever the plugin actually resolves - if
  gRPC-related build errors mention symbol/ABI mismatches, check
  `GRPC_CPP_PLUGIN`/`GRPC_PREFIX` in the Makefile before assuming the code
  is wrong.
- `libgmock-dev` is not installed, and Claude has no `sudo` in this
  environment - if GMock-based tests are needed, ask the user to run `sudo
  apt install -y libgmock-dev` themselves (same pattern already used for
  `protobuf-compiler-grpc` and `libgtest-dev` earlier in the project).
- `gh` CLI is not installed - can't open PRs via `gh pr create`; instead
  push the branch and hand the user the `.../pull/new/<branch>` URL git
  itself prints on push.
- A clean build has occasionally hit a transient GCC internal compiler
  error (ICE) that didn't reproduce on retry - if a build fails with an ICE
  ("Please submit a full bug report"), retry once before assuming the code
  is at fault.

## Current task snapshot

Check the live task list first (`TaskList` tool) - this is a point-in-time
note, not authoritative. As of the last update:

**Done**: MVP bring-up (build recovery, `BouncingBall`), type system stages
1 & 2 (byte-oriented `Value`/`Layout`/`TypeRegistry`, wired into
`Model`/`SimAdapter`), GTest package, `PIDController` + `MassSpringDamper`
models, `Simulator` orchestration (routing, lock-step stepping, composition
validation stopgap), the `Solver` fixed-step-size fix.

**Pending**: gRPC `Simulation` service (component-level RPC) + client
adapter, FMU adapter (dlopen-based), `Simulator`'s gRPC service
(orchestrator-to-orchestrator stepping), sim executable runner (JSON config
-> loads a `System` -> runs it), extracting the composition-validation
stopgap out of `Simulator` into its own component (name TBD - candidates
discussed: `SimulationLinker`), more simple control-system models building
incrementally toward a vehicle model, GitHub Actions CI running build+test.
