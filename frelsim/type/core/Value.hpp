#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "frelsim/proto/Type.pb.h"

namespace frelsim::type::core {

class TypeRegistry;

/**
 * \brief A Value is raw bytes plus the Type that describes how to interpret
 * them - not a tree of tagged sub-values. Encoding is little-endian,
 * independent of host byte order (see Layout.hpp), so it's safe to move
 * between processes as well as within one, and matches what a struct/array
 * field access (getField/getElement) slices directly out of `bytes_`.
 *
 * A struct or array Value's type is commonly a type_ref (see Type.proto)
 * naming a schema in a TypeRegistry, rather than an inline description -
 * getField/getElement/etc. take a TypeRegistry to resolve it.
 */
class Value final {
    public:

        Value() = default;

        Value(proto::Type type, std::vector<std::byte> bytes);

        ~Value() = default;

        proto::Type const& getType() const;

        std::vector<std::byte> const& getBytes() const;

        void setBytes(std::vector<std::byte> bytes);

        void setType(proto::Type type);

        // ----- Scalar factories/accessors -----
        // Truncates/widens as needed; width/precision select how many bytes
        // are actually stored (e.g. makeInt(x, 32) stores 4 bytes).

        static Value makeInt(std::int64_t value, int width = 64, bool isSigned = true);

        static Value makeBool(bool value);

        static Value makeDouble(double value, int precision = 64);

        static Value makeString(std::string value);

        // \pre getType() is an IntegerType.
        std::int64_t asInt() const;

        // \pre getType() is a BoolType.
        bool asBool() const;

        // \pre getType() is a FloatType.
        double asDouble() const;

        // \pre getType() is a StringType. bytes are interpreted directly as
        // string content (no length prefix - getBytes().size() is the length).
        std::string asString() const;

        // ----- Struct/array field access -----
        // These resolve getType() (following a type_ref if present) against
        // `registry` to find the requested field/element's offset and size,
        // then slice/splice that many bytes out of / into bytes_.

        // \pre getType() resolves to a StructType with a field named `fieldName`.
        Value getField(std::string const& fieldName, TypeRegistry const& registry) const;

        // \pre getType() resolves to a StructType with a field named
        // `fieldName` whose layout matches `fieldValue`'s bytes exactly.
        void setField(std::string const& fieldName, Value const& fieldValue, TypeRegistry const& registry);

        // \pre getType() resolves to an ArrayType and index < arrayLength().
        Value getElement(std::size_t index, TypeRegistry const& registry) const;

        // \pre getType() resolves to an ArrayType, index < arrayLength(), and
        // elementValue's bytes match the array's element size exactly.
        void setElement(std::size_t index, Value const& elementValue, TypeRegistry const& registry);

        // \pre getType() resolves to an ArrayType.
        std::size_t arrayLength(TypeRegistry const& registry) const;

    private:
        proto::Type type_;

        std::vector<std::byte> bytes_;

};

}
