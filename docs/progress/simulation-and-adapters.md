# Simulation and adapters (`frelsim/simulation/`, `frelsim/adapt/`)

`Simulation` is one composed unit of co-simulation (wraps a `SimAdapter`,
which in turn wraps either a `Model` via `ModelAdapter`, or eventually an FMU
or handwritten-code target). This is the abstraction `Linker` constructs one
of per `RoutedSimulation` entry, and `Simulator` steps/routes between.

## Bugs fixed in passing (orchestration track)

Found because linking with `--whole-archive` for the first time actually
force-linked code nothing had exercised before (see
[`build-and-tooling.md`](build-and-tooling.md)):

- `Simulation`'s constructor never actually built `simAdapter_` - a
  `stepUntil`/`get`/`set` on any `Simulation` was a null-pointer dereference
  waiting to happen.
- Its destructor was declared but never defined (link error the moment one
  was destroyed).
- It never exposed `guaranteeUntil` at all, despite `SimAdapter` having had
  it since the MVP track.

All three were latent from early scaffolding and had simply never been
exercised by any real running code until `Simulator` actually composed and
stepped a `Simulation` for the first time.
