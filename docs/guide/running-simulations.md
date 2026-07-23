# Running a simulation

`Overseer` (`frelsim/overseer/Overseer.hpp`) is the single entry point -
it owns compiling a config, linking the composition, and executing it (see
[`architecture.md`](architecture.md)), and exposes two ways to run.

## From the command line

```bash
make sim
./build/frelsim_sim path/to/config.json
```

`sim/main.cpp` is deliberately thin - it just constructs an `Overseer` from
`argv[1]`, calls `initialize()`, then `sim()` (run-to-completion), and
prints the final simulation time or a clear error to stderr with a non-zero
exit code. Try it against the checked-in example:

```bash
./build/frelsim_sim examples/pid_mass_spring_damper.json
# Simulation complete at t=0.5
```

## From C++ code

```cpp
#include "frelsim/overseer/Overseer.hpp"

// From a config file - Overseer runs Compiler itself.
frelsim::overseer::Overseer overseer("path/to/config.json");
overseer.initialize();
overseer.sim(); // run to completion

// Or, if you already have a parsed System (e.g. built in-memory, or
// received from a future gRPC CreateSimulatorRequest):
frelsim::overseer::Overseer overseer2(system); // sim::proto::System
overseer2.initialize();
```

## Two run modes

- **`sim()`** - run-to-completion. Steps until the whole simulation reaches
  `stop_time`.
- **`step(stopTime)`** - single-step, externally-driven. Advances by *at
  most* `max_step_size` per call (a conservative lock-step bound) and
  returns whether the whole simulation has finished - `stopTime` here is a
  cap on that one step's horizon, not a jump target. Reaching a distant
  target takes repeated calls:

```cpp
bool finished = false;
while (!finished) {
    finished = overseer.step(targetTime);
    // inspect intermediate state here if you want, e.g.:
    // Values v = overseer.get("plant", {someOutputIdentifier});
}
```

This is the mode a future gRPC/REST caller (or any external controller)
would use to drive a simulation one call at a time instead of letting it
run unattended - `Overseer`'s method names deliberately mirror
`frelsim/proto/Simulator.proto`'s (not yet implemented) gRPC service
one-to-one, so it's already shaped to become that service's backing
implementation. See [`../progress/pipeline.md`](../progress/pipeline.md)
for the full mapping and the one known gap (no multi-session support yet).

## Inspecting a running/paused simulation

```cpp
Values values = overseer.get("componentKey", identifiers);
double t = overseer.simulationTime();
overseer.pause();  // sim() will pause before its next step
overseer.resume();
```

All of these throw `std::logic_error` if called before `initialize()`.

## Verifying results: logging a time series

`overseer.get()` only gives a point-in-time snapshot - to check a
simulation's *trajectory* is actually correct (a step response converging
the way it should, a vehicle reaching the right terminal velocity), record
a time series instead. Add to your `System` config:

```json
{
    "logged_outputs": [
        {"domain": "plant", "scope": "Output", "name": "position"},
        {"domain": "plant", "scope": "Output", "name": "velocity"}
    ],
    "log_path": "output/my_run_log.csv"
}
```

`Overseer::sim()` writes the recorded time series to `log_path`
automatically once the run completes - `frelsim_sim` needs no extra flags
or code, since this is entirely config-driven (see
`examples/pid_mass_spring_damper.json`, which already does this). Only
numeric identifiers (`Output`/`Parameter`, `Float`/`Integer`/`Bool` typed)
can be logged.

If you're driving a simulation via `step()` instead of `sim()`, `log_path`
isn't auto-written (each `step()` call shouldn't rewrite the whole file) -
call `overseer.writeLog(path)` explicitly once you're done:

```cpp
Overseer overseer(system); // logged_outputs set, log_path left unset
overseer.initialize();
while (!overseer.step(targetTime)) { /* ... */ }
overseer.writeLog("output/my_run_log.csv");
```

Then plot it:

```bash
python3 scripts/plot_results.py output/my_run_log.csv
python3 scripts/plot_results.py output/my_run_log.csv --save plot.png
```

See [`../progress/recorder.md`](../progress/recorder.md) for the design
(and why this is CSV/file-based rather than a live streaming connection -
that's a natural future extension once a gRPC service actually exists to
stream over, not a dead end).

## Building your own config

See [`composition.md`](composition.md) for the full `System` format, and
[`models.md`](models.md) for what each built-in model expects. The
`initial_parameters`/`Value` encoding (a `Type` descriptor + raw
little-endian bytes) is the same wire format described in
[`../progress/type-system.md`](../progress/type-system.md) - the easiest
way to build one by hand is to copy the shape from
`examples/pid_mass_spring_damper.json` and adjust the numbers.
