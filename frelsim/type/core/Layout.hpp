#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "frelsim/proto/Type.pb.h"

namespace frelsim::type::core {

class TypeRegistry;

/**
 * \brief The offset of one field within a laid-out StructType, alongside its
 * own (already-resolved, never a type_ref) Type.
 */
struct FieldLayout {
    std::string name;
    std::size_t offset;
    proto::Type type;
};

/**
 * \brief The computed size/alignment/layout of a Type, using standard C
 * struct layout rules: each field is placed at the next offset that is a
 * multiple of its own alignment, and the whole struct's size is padded up to
 * a multiple of its largest member's alignment. Encoding is little-endian
 * throughout, independent of host byte order, so it's safe to move between
 * processes (e.g. over gRPC) as well as within one.
 *
 * `fields` is only populated when the described Type is a StructType.
 * `elementStride`/`elementCount` are only populated when it's an ArrayType.
 * A StringType has size 0 here - it's variable-length and is not describable
 * by a fixed layout, which is also why it cannot appear as a struct field
 * (see computeLayout()).
 */
struct Layout {
    std::size_t size = 0;
    std::size_t alignment = 1;

    std::vector<FieldLayout> fields;

    std::size_t elementStride = 0;
    std::size_t elementCount = 0;

    // True only for a (possibly type_ref-indirected) StringType. Checked
    // whenever a Layout is about to be nested inside a struct field or array
    // element, since those require a fixed size - see computeLayout().
    bool isVariableLength = false;
};

/**
 * \brief Compute the Layout for `type`. Recurses into struct fields/array
 * element types, resolving any type_ref via `registry`.
 * \throws std::invalid_argument if `type` has no case set, if a struct field
 * is (directly or transitively) a variable-length StringType, or if a
 * type_ref names a type that isn't registered.
 */
Layout computeLayout(proto::Type const& type, TypeRegistry const& registry);

} // frelsim::type::core
