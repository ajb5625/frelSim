# Sim executable runner (`sim/`)

Task #10, made small by the Compiler/Linker/Overseer work landing first
(see [`pipeline.md`](pipeline.md)): `Overseer` already owns all three
simulation stages internally, so this executable (`sim/main.cpp`, built as
`frelsim_sim`) has no parsing or validation logic of its own - it just
constructs an `Overseer` from `argv[1]`, calls `initialize()` then `sim()`,
and reports the final `simulationTime()`. Deliberately run-to-completion
only; single-step control (`Overseer::step()`) is for a future gRPC/REST
caller, not this executable. Catches exceptions from any of the three
stages and reports them to stderr with a non-zero exit code rather than an
unhandled-exception crash.

Build details (Makefile `sim` target, `--whole-archive` requirement) are in
[`build-and-tooling.md`](build-and-tooling.md).

Added `examples/pid_mass_spring_damper.json` - a real config for the
PID-driving-`MassSpringDamper` composition from the control-loop-demo track
(see [`simulator.md`](simulator.md)), serialized as actual JSON (not just
constructed in-memory in a test) to prove the whole path (JSON file ->
`Compiler` -> `Linker` -> `Simulator`) works end-to-end:
`./build/frelsim_sim examples/pid_mass_spring_damper.json` runs to
completion and reports `t=0.5`. Also manually verified the error paths -
no args, a missing file, and a malformed composition (unknown source) -
each fail with a clear message and exit code 1 rather than a crash. CI now
builds `frelsim_sim` and runs it against this example config on every
push/PR, so this path stays covered going forward.

No unit tests for `main()` itself - there's no existing precedent in this
codebase for testing process entry points, and the underlying logic
(`Compiler`/`Linker`/`Overseer`) is already covered elsewhere; the example
config + CI run function as the executable's own integration test.
