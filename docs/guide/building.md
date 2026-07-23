# Building and testing

## Requirements

- A C++20 compiler (developed against `g++`)
- `protoc` (protobuf compiler) + `libprotobuf-dev` - **not** `protobuf-lite`,
  since `Compiler` uses `google::protobuf::util::JsonStringToMessage` from
  `json_util.h`
- `libgrpc++-dev` + `protobuf-compiler-grpc` (`grpc_cpp_plugin`) - gRPC
  services aren't implemented yet, but the proto stubs are already
  generated and linked
- `libeigen3-dev` - used throughout the solver hierarchy (`State`/`Matrix`
  are `Eigen::VectorXd`/`Eigen::MatrixXd`)
- `libgtest-dev` for `make test`; `libgmock-dev` is optional (see below)

## Commands

```bash
make            # builds build/libfrelsim.a
make test       # builds + runs the GoogleTest suite (build/frelsim_tests)
make sim        # builds the sim executable runner (build/frelsim_sim)
make clean      # removes build/ - do this before a build after switching
                # branches with structural changes, even though header
                # dependency tracking (-MMD -MP) is in place
```

`make`/`make test` link a lot of gRPC/abseil transitively even though gRPC
services aren't used yet - expect the link step alone to take a minute or
two, especially for `frelsim_tests`.

## GMock

`libgmock-dev` is optional: the Makefile detects it via `pkg-config` and
skips any test file with `Mock` in its name if it's not installed, so
`make test` works either way. Nothing in the current test suite needs it -
every test target so far is concrete/state-based rather than an interface
that needs mocking.

## A note on self-registration

`Model` subclasses (`BouncingBall`, `PIDController`, etc.) and `Solver`
subclasses (`Euler`, `DormandPrince45`, etc.) register themselves with
their factories via a static initializer in their own `.cpp`
(`FRELSIM_REGISTER_MODEL`/`FRELSIM_REGISTER_SOLVER`) - this is why adding a
new model or solver never requires editing `ModelFactory`/`SolverFactory`.

The cost: because `libfrelsim` is a static archive, a plain `ar`-built `.a`
would silently drop any `.o` nothing else references (which is every
self-registering model/solver, by design) - so its registration would just
never run. Both `frelsim_tests` and `frelsim_sim` are linked against
`libfrelsim.a` via `-Wl,--whole-archive ... -Wl,--no-whole-archive`
(`LINK_LIBFRELSIM` in the Makefile) specifically to prevent this. If you
ever add a new executable that needs the model/solver registries populated
(a name lookup returning unexpectedly `nullptr` is the symptom), link it the
same way.
