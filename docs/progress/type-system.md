# Type system (`frelsim/type/`)

`frelsim/type/core/` (`Value`, `Layout`, `TypeRegistry`, `TypeUtil`) and
`frelsim/type/marshal/` (`Marshaler`).

## Stage 1: the type system itself, in isolation

Goal is int, bool, double, string, struct, and array. Deliberately staged
separately from wiring it into the rest of the framework (see Stage 2
below) to keep the diff reviewable and the type system independently
testable before it ripples through every interface.

**Design, revised mid-implementation.** The first pass built `Value` as a
tree of heap-allocated, tagged-union nodes (`std::variant` over scalars plus
`map<string,Value>`/`vector<Value>` for struct/array) - convenient in-process
C++, but the wrong shape for what this framework actually needs to do: hand
values across process boundaries to FMUs and handwritten-code components,
which want raw byte buffers with a known layout, not a tree to walk. Reworked
before it was ever committed to:

- `Value` is now just `{ Type type; bytes data; }` - raw bytes plus a type
  descriptor, both on the proto wire (`Value.proto`) and in C++
  (`frelsim::type::core::Value`). No more `StructValue`/`ArrayValue` messages,
  no more per-scalar-type oneof fields.
- `StructType`/`ArrayType` are schemas only (field names + types, or element
  type + fixed dimensions) - they don't carry data or precomputed offsets.
  `frelsim::type::core::Layout` (`Layout.hpp`/`.cpp`) computes size,
  alignment, and (for structs) each field's offset from a `Type`, using
  standard C struct layout rules: a field goes at the next offset that's a
  multiple of its own alignment, and the whole struct is padded to a multiple
  of its largest member's alignment. Scalars are self-aligned (size ==
  alignment). Encoding is explicitly little-endian regardless of host byte
  order, so a `Value`'s bytes are safe to send over gRPC between potentially
  different machines, not just interpretable on the machine that produced
  them.
- `frelsim::type::core::TypeRegistry` (`TypeRegistry.hpp`/`.cpp`) registers a
  schema once under a URI (same `domain.scope.name` convention as
  `Identifier`) and caches its computed `Layout`, so many `Value`s referencing
  the same struct (e.g. an array of a struct type) don't each repeat the
  schema - they just carry a `type_ref` (a `Type` oneof case: `string
  type_ref`) naming the registry entry. Scoped to a `System` composition so
  multiple components can agree on and reference the same wire-compatible
  struct layout.
- Struct fields must be fixed-size for now - a `string` (or anything that
  resolves to one via `type_ref`) is rejected as a struct field or array
  element by `computeLayout`, since "typical C struct" offset computation
  assumes every field has a known size. A `string` can still be a top-level
  `Value` on its own, just not nested. No indirection scheme (offset+length
  into a trailing variable-length region, `FlatBuffers`/`Cap'n Proto`-style)
  for now.
- `Value::makeInt/makeBool/makeDouble/makeString` encode a scalar straight to
  its little-endian bytes (via `std::bit_cast` for floats); `asInt/asBool/
  asDouble/asString` decode back, throwing if the stored `Type` doesn't match
  what's being asked for. `getField`/`setField`/`getElement`/`setElement`/
  `arrayLength` resolve a struct/array `Value`'s `Type` (following a
  `type_ref` if present) against a `TypeRegistry` to find the requested
  field/element's offset and size, then slice/splice that many bytes -
  including nested access (an array of structs: `getElement` then `getField`
  on the result).
- One correction to the original proto sketch this replaced:
  `IntegerType.signed` was renamed to `is_signed` - `signed` is a C++ keyword
  and cannot be used as a generated accessor method name (`bool signed()
  const` doesn't compile).

`Marshaler::protoToCpp`/`cppToProto` are a near-trivial transcode (proto
`bytes` <-> `std::vector<std::byte>`), since proto and C++ `Value` are
basically the same shape - no recursion needed at the marshaling layer at
all, interpretation happens later via `Value`'s own accessors. Also fixed a
pre-existing bug found while first rewriting this (before the byte-oriented
redesign, but the fix carried forward): `cppToProto` was declared as a
`Marshaler` member in the header but defined as a free function (missing the
`Marshaler::` qualifier) in the `.cpp` - the member was never actually
implemented, so calling it would have been a link error the first time
anything tried.

## Stage 2: wiring typed values into the framework

`Values`/`Parameters`/`SimValue`/`SetOperations` (`Aliases.hpp`) now alias
`type::core::Value` instead of raw `double`. This turned out to touch very
little actual logic - `SimAdapter`, `Model`, `ModelAdapter`, `Simulation`,
`Event`/`EventEngine` all already treated these as opaque pass-through types
(signatures and container operations, no arithmetic on the values
themselves), so they just needed to recompile against the new alias. The
only real code changes were:

- `BouncingBall::getOutputs`/`getParameters` now wrap results in
  `type::core::Value::makeDouble(...)`; `setParameters` now reads via
  `value.asDouble()` instead of assigning a raw double directly.
- `Simulation.proto`'s `GetResponse.values` and `SetOperation.value` changed
  from `double` to `frelsim.type.proto.Value` (this is exactly what a
  pre-existing `// TODO use custom typesystem later.` comment on
  `GetRequest` had been pointing at).

Verified two ways: the full test suite still passes unchanged, and
re-running the same manual `BouncingBall` driver program (updated to read
`.asDouble()` off the typed `Values` now returned by `SimAdapter::get`)
produces bit-for-bit the same trajectory as before the swap - confirming
this was a pure representation change, not a behavior change.

## `typesEqual` (added alongside the compile/link/execute pipeline work)

The composition-wiring dry-run (see [`pipeline.md`](pipeline.md)) originally
relied on an incidental exception (e.g. `Value::asDouble()` rejecting a
non-FloatType) to catch a type mismatch between a wired source and
destination - it never actually compared types, so it only caught some
mismatches by accident. Fixed by adding
`frelsim::type::core::typesEqual(Type const&, Type const&)`
(`TypeUtil.hpp/.cpp`) - structural equality across every `Type` oneof case:
`IntegerType` compares `is_signed`+`width`, `FloatType` compares
`precision`, `BoolType`/`StringType` are trivially equal to themselves,
`StructType`/`ArrayType` recurse field-by-field/element-by-element, and a
`type_ref` compares by URI string only.

That last point is a deliberate limitation, not an oversight: a `type_ref`
is never resolved against an inline description of the same type, since
that would need a `TypeRegistry` this utility doesn't take (mirroring
`computeLayout`'s own registry-taking design in `Layout.hpp`). Two
components wiring the same type should register it once and both reference
it by `type_ref`, rather than relying on this utility to see through one
side describing it inline.

Tested with `TypeUtilTest` (9 cases) covering every oneof case plus the
deliberate type_ref-vs-inline non-resolution and an unset-type guard.
