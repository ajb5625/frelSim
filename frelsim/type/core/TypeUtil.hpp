#pragma once

#include "frelsim/proto/Type.pb.h"

namespace frelsim::type::core {

/**
 * \brief Structural equality between two Types: same oneof case, and same
 * shape within that case (IntegerType compares is_signed+width, FloatType
 * compares precision, StructType compares fields recursively by name and
 * type in order, ArrayType compares base_type recursively and dimensions).
 *
 * A type_ref is compared by URI string only - two type_refs naming the same
 * registered type compare equal, but a type_ref is never resolved against an
 * inline description of the same type (that would need a TypeRegistry to
 * resolve against, which this deliberately doesn't take - see
 * computeLayout() in Layout.hpp for the analogous, registry-taking case).
 * Callers wiring components that might describe the same type both ways
 * should register it once and reference it by type_ref on both ends rather
 * than relying on this to see through the difference.
 */
bool typesEqual(proto::Type const& a, proto::Type const& b);

} // namespace frelsim::type::core
