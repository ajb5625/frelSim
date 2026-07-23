# Time-series logging (`frelsim/recorder/`)

Until this track, the only way to inspect a simulation's results was
`Overseer::get()` - a single point-in-time snapshot. There was no way to
verify a simulation's *trajectory* is actually correct (a step response
converging correctly, a vehicle reaching the right terminal velocity,
etc.) without stepping through it by hand. This is the gap that motivated
the whole track: "how do we check these results are actually correct."

## Design

Two ways were considered: write a time series to a file (CSV, offline
verification/plotting), or stream it over a live connection. Went with
CSV first, since no gRPC service exists yet to stream over (see tasks #3/
#5) - but the design deliberately keeps the streaming door open: `Recorder`
doesn't know about `Simulator` at all, it just takes a `Getter` callback
(`function<Values(domain, ids)>`) to sample values with. A future streaming
handler could drive the same `Recorder`, or bypass it entirely and reuse
just the `Simulator` step-observer hook below with a different sink.

**`Recorder`** (`frelsim/recorder/Recorder.hpp/.cpp`) accumulates one row
per `record(time, getter)` call, one column per watched identifier -
`domain.scope.name`, same convention as composition wiring. Groups watched
identifiers by domain so a component with several watched fields is only
queried once per `record()` call, not once per field. Values are coerced to
`double` (`FloatType` directly, `IntegerType`/`BoolType` cast) - a
string/struct/array watched identifier throws, since there's no sensible
way to plot one as a time series.

**`Simulator::setStepObserver`** - a new hook, invoked at the end of every
`step()` call (including the bootstrap step, and every call made from
within `sim()`'s own internal loop). This was the one gap that needed
closing to make recording work uniformly in both run modes: `Overseer::sim()`
just forwards to `Simulator::sim()`, which loops internally - without this
hook, `Overseer` would only ever see per-step state during an
externally-driven `step()`-based run, not during `sim()`. The observer
receives the `Simulator` itself (not a value), so it can call
`get()`/`simulationTime()` to sample whatever it needs - `Simulator` still
doesn't know anything about `Recorder`, keeping the same separation of
concerns as the rest of this codebase (`Simulator` does execution only).

**`System` proto gained two fields**: `logged_outputs` (identifiers to
watch - empty means recording is entirely skipped, so the common case pays
nothing for it) and `log_path` (optional - if set, `Overseer::sim()` writes
the CSV automatically once the run completes). `Overseer::initialize()`
constructs a `Recorder` from `logged_outputs` if non-empty and wires it as
the `Simulator`'s step observer. A `step()`-driven caller isn't
auto-written (each `step()` call shouldn't rewrite the whole file) - it
calls `Overseer::writeLog(path)` explicitly once it's done instead.

## Verification

Added `logged_outputs`/`log_path` to `examples/pid_mass_spring_damper.json`
(watching the plant's `position`/`velocity` and the PID's `output`) and ran
it for real: `./build/frelsim_sim examples/pid_mass_spring_damper.json`
produces `output/pid_mass_spring_damper_log.csv` - 31 rows, position
converging toward the setpoint of 5 as expected, matching the physical
behavior already verified in `SimulatorTest`/`OverseerTest`. CI now checks
this file gets written (`test -s ...`) on every push/PR.

`scripts/plot_results.py` reads a `Recorder` CSV and plots every column
against time via matplotlib - the actual "verify this looks right"
consumer. matplotlib wasn't installed in the dev environment this was
built in, so the plotting call itself couldn't be executed here; the CSV
*parsing* logic (`load_csv`) was verified directly against the real
generated log above (correct header, correct row/value data) with
matplotlib's import stubbed out, but rendering a real plot should be
checked once matplotlib is available.

## Testing

`RecorderTest` (row accumulation, CSV header/row format, per-domain getter
batching, numeric type coercion, and the non-numeric-value / can't-open-path
error paths); extended `SimulatorTest` (the step observer firing once per
`step()` call, in both run modes) and `OverseerTest` (config-driven
auto-write on `sim()` completion, explicit `writeLog()` after a
`step()`-driven run, and the "nothing configured" error path).
