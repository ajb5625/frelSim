# Composing a System

A `System` (`frelsim/proto/System.proto`) describes a whole composition:
which components to construct, their initial parameters, and how they're
wired together.

```proto
message System {
    Identifier identifier = 1;
    double stop_time = 2;
    double max_step_size = 3;
    repeated RoutedSimulation composition = 4;
}

message RoutedSimulation {
    Identifier simulation = 1;
    SimulationDescription sim_description = 2;
    Identifier source = 3;
    repeated Identifier destinations = 4;
    repeated SetOperation initial_parameters = 5;
}
```

## Constructing a component

Each `RoutedSimulation` entry constructs one component: `simulation.domain`
is its key in the composition (what other entries' `source`/`destinations`
reference), and `sim_description` is its full `SimulationDescription` -
`cosim_target` (`Model` for a built-in model), `model_spec.component_name`
(the name it self-registered under - see [`models.md`](models.md)),
`model_spec.solver_type`, and its `task` (type/period/offset/max_step_size).

## Wiring an edge

`source` and `destinations` describe **one wired edge**: read `source`'s
value, apply it to every identifier in `destinations`. Each identifier is
`domain.scope.name` (`domain` = a component's key, `scope` =
`"Output"`/`"Input"`/`"Parameter"`, `name` = the specific field on that
component - see [`models.md`](models.md) for each model's field names).

**A real gotcha: the edge isn't tied to that entry's own `simulation`
field.** `Simulator`/`Linker` only look at the literal `source`/
`destinations` values across the *whole* composition list - which
`RoutedSimulation` entry they happen to sit under doesn't matter for
routing. In practice, put each component's own "where does my input come
from" edge on that component's own entry, matching the proto's documented
intent, but know that it's not enforced - a two-component closed loop looks
like:

```json
{
  "stop_time": 0.5,
  "max_step_size": 0.02,
  "composition": [
    {
      "simulation": {"domain": "pid"},
      "sim_description": { "...": "PIDController, see below" },
      "source": {"domain": "plant", "scope": "Output", "name": "position"},
      "destinations": [{"domain": "pid", "scope": "Input", "name": "measurement"}],
      "initial_parameters": [
        {"id": {"scope": "Parameter", "name": "setpoint"}, "value": { "...": "5.0" }}
      ]
    },
    {
      "simulation": {"domain": "plant"},
      "sim_description": { "...": "MassSpringDamper, see below" },
      "source": {"domain": "pid", "scope": "Output", "name": "output"},
      "destinations": [{"domain": "plant", "scope": "Input", "name": "force"}]
    }
  ]
}
```

(The full runnable version of this is `examples/pid_mass_spring_damper.json`
- see [`running-simulations.md`](running-simulations.md).)

## A real gotcha: scope {#a-real-gotcha-scope}

`ModelAdapter::get`/`set` dispatch purely by an identifier's `scope` -
`"Output"`/`"Input"` go to `getOutputs`/`setInputs`(`getInputs`),
`"Parameter"` goes to `getParameters`/`setParameters`. **An identifier with
no scope set (or a typo'd one) is silently dropped, not an error** - it
just reads back empty or does nothing. If a wired edge or an
`initial_parameters` entry doesn't seem to be taking effect, check `scope`
first.

## Initial parameters

`initial_parameters` on a `RoutedSimulation` are applied once, right after
that component is constructed (by `Linker`, before `Simulator` ever exists)
- this is the only way to configure a component's starting values (PID
gains, a setpoint, a plant's mass) through a `System`, since a fresh model
otherwise starts at its hardcoded defaults (see the tables in
[`models.md`](models.md)).

## Wiring validation

`Linker` (not `Simulator`) validates the whole composition once, before
execution ever starts:

- An unknown `source`/`destinations` domain (no such component was
  constructed) - fails immediately.
- A source that produces no value.
- **A genuine type mismatch** between a source's output type and a
  destination's input type, via `typesEqual` (structural comparison, not an
  incidental exception) - see
  [`../progress/pipeline.md`](../progress/pipeline.md) and
  [`../progress/type-system.md`](../progress/type-system.md).
- A destination that doesn't support type introspection at all (didn't
  override `Model::getInputs`) - fails loudly, since an unobserved type
  can't be verified. If you write a model with an input, implement
  `getInputs` (see [`models.md`](models.md)).

All of this happens at "link time," before a `Simulator` exists - a broken
composition never gets partway into a run before failing.
