# frelsim progress log

Design decisions, bugs found and their root causes, and things tried and
reverted - split into one file per area of the codebase under
[`progress/`](progress/), organized by directory rather than strictly by
date. Each file is still internally chronological (oldest work at the top).

For how to actually *use* frelsim, see the [user's guide](guide/README.md)
instead - this log is about *why* things are the way they are, not how to
drive them.

| File | Covers |
|---|---|
| [`progress/build-and-tooling.md`](progress/build-and-tooling.md) | Repo recovery, `Makefile`, CI, dev-environment quirks - not any one `frelsim/` subdirectory |
| [`progress/type-system.md`](progress/type-system.md) | `frelsim/type/` - `Value`/`Layout`/`TypeRegistry`/`TypeUtil`/`Marshaler` |
| [`progress/scheduling-and-events.md`](progress/scheduling-and-events.md) | `frelsim/task/`, `frelsim/schedule/`, `frelsim/event/`, `frelsim/util/` |
| [`progress/models.md`](progress/models.md) | `frelsim/model/` - `Model`, `ModelFactory`, `ModelAdapter`, and every concrete model |
| [`progress/simulation-and-adapters.md`](progress/simulation-and-adapters.md) | `frelsim/simulation/`, `frelsim/adapt/` |
| [`progress/solvers.md`](progress/solvers.md) | `frelsim/integrate/` - the `Solver` hierarchy, `SolverFactory`, stiffness detection |
| [`progress/simulator.md`](progress/simulator.md) | `frelsim/simulator/` - routing, lock-step time advancement |
| [`progress/pipeline.md`](progress/pipeline.md) | `frelsim/compiler/`, `frelsim/linker/`, `frelsim/overseer/` - the config-read → link → execute pipeline |
| [`progress/sim-executable.md`](progress/sim-executable.md) | `sim/` - the `frelsim_sim` executable runner |

Not yet built (check the live task list for current status, not this file):
gRPC `Simulation`/`Simulator` services, FMU adapter, REST layer.
