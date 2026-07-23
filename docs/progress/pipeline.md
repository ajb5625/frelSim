# Compile → link → execute pipeline (`frelsim/compiler/`, `frelsim/linker/`, `frelsim/overseer/`)

Task #12 (extract composition validation out of `Simulator`) turned into a
broader cut once the three-stage architecture (config read -> link ->
execute) got designed alongside it: each stage is now its own component, and
a top-level `Overseer` owns invoking all three and exposes both a
run-to-completion and a single-step control mode.

## `Compiler` - stage 1

`frelsim/compiler/Compiler.hpp/.cpp`. Reads a JSON config file and parses
it into a structural `System` proto via
`google::protobuf::util::JsonStringToMessage` (full `libprotobuf` was
already linked, not `-lite`, so `json_util.h` was available with no build
changes needed). Purely structural - no knowledge of composition semantics -
throwing `std::invalid_argument` on a missing file or a JSON payload that
doesn't conform to `System`'s shape.

## `Linker` - stage 2

`frelsim/linker/Linker.hpp/.cpp`. This is what task #12 actually became:
`Simulator::initialize()`'s construction logic and
`Simulator::validateComposition()`'s dry-run wiring check (see
[`simulator.md`](simulator.md) for that history) moved out near-verbatim
into `Linker::link()`, which constructs every composed `Simulation` from the
`System`'s composition, applies `initial_parameters`, and validates the
wiring (unknown source/destination references, a source producing no
value, a type mismatch surfacing from a destination's `set()`) - all before
a `Simulator` ever exists, not inside its runtime loop. Returns a
`LinkedSystem` (the constructed `simulations` map + `system` metadata).
`Simulator`'s constructor now takes a `LinkedSystem` directly;
`Simulator::initialize()` shrank to just resetting `simulationTime_`, and
`validateComposition()` was deleted entirely from `Simulator`.

**Real type checking (post-review fix).** The dry-run above originally
didn't actually type-check anything - it just hoped a mismatch would happen
to throw partway through a destination's `set()` (e.g. `Value::asDouble()`
rejecting a non-FloatType), which only ever caught some mismatches by
accident, never by an explicit comparison. Fixed properly - see
[`type-system.md`](type-system.md) for `typesEqual` itself and
[`models.md`](models.md) for `Model::getInputs` (the gap this exposed: there
was no way to even ask a destination what type it expects). `Linker::link()`
now fetches the destination's current input value before wiring anything,
and requires `typesEqual` against the source's actual output type - a
destination that doesn't override `getInputs` (e.g. `BouncingBall`, which
takes none) fails loudly rather than silently skipping the check, since an
unobserved type can't be verified.

Tests: `LinkerTest` covers a valid two-component link, unknown
source/unknown destination, a genuine cross-type mismatch (needed a tiny
test-only self-registering `BoolOutputStub` model, since every real
registered model in this codebase happens to use only `FloatType` fields),
and the "destination can't be type-checked" failure path via `BouncingBall`.

## `Overseer` - the top-level entry point

`frelsim/overseer/Overseer.hpp/.cpp`. Name chosen together with the user
after a long naming back-and-forth, landing on this Fallout-flavored pick.
Owns all three stages and mirrors `frelsim/proto/Simulator.proto`'s
(currently unimplemented, task #5) gRPC service one-to-one -
`CreateSimulator`/`Initialize`/`Sim`/`StepUntil`/`Terminate` map directly
onto `Overseer`'s two constructors and
`initialize()`/`sim()`/`step()`/`terminate()` - so it's positioned to become
that service's backing implementation later, and is usable standalone (no
gRPC) by the sim executable runner (see
[`sim-executable.md`](sim-executable.md)) in the meantime.

Two constructors: one from a config path (runs `Compiler` itself - the sim
executable runner never parses anything itself) and one from an
already-built `System` (for a future gRPC handler, where protobuf already
deserialized the request off the wire - nothing left for `Compiler` to do).
Every lifecycle method after `initialize()` throws `std::logic_error` if
called too early, via a private `requireSimulator()` accessor (renamed from
an earlier `requireInitialized()` - the old name didn't say what it
actually returns, just that it guards something) rather than
null-dereferencing.

**Flagged-but-deliberately-unsolved gap**: `Simulator.proto`'s RPCs carry no
session/id parameter, meaning the proto as written assumes one `Overseer`
per service instance rather than several concurrent simulations sharing one
server process. If that's ever needed, it's a `map<sessionId,
unique_ptr<Overseer>>` living in the service layer above `Overseer` (or a
session id added to the proto) - `Overseer` itself doesn't need to change
either way.

## Testing

`Simulator` had zero test coverage before this pipeline existed (no
`SimulatorTest.cpp` at all); added one alongside `CompilerTest`,
`LinkerTest`, and `OverseerTest` - the last exercises both constructors and
both run modes end-to-end (the PID-driving-`MassSpringDamper` composition
from the control-loop-demo track, run once via `sim()` and once via
repeated `step()` calls, checking they reach the same final state).

One thing that tripped up writing these: `ModelAdapter::get`/`set` dispatch
to `getOutputs`/`setInputs`/`getParameters`/`setParameters` purely by an
identifier's `scope` (`"Output"`/`"Input"`/`"Parameter"`) - an identifier
with no scope set is silently dropped by both rather than erroring, which
looked like a wiring bug in the new composition fixtures before the actual
cause (missing `scope`) was found.

Full suite as of this track (with the post-review type-checking fix):
112/112 passing.
