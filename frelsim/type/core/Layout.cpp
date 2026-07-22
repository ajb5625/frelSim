#include "Layout.hpp"
#include "TypeRegistry.hpp"
#include <algorithm>
#include <stdexcept>

namespace frelsim::type::core {

namespace {

std::size_t roundUp(std::size_t offset, std::size_t alignment) {
    if (alignment <= 1) {
        return offset;
    }
    return (offset + alignment - 1) / alignment * alignment;
}

} // namespace

Layout computeLayout(proto::Type const& type, TypeRegistry const& registry) {
    switch (type.type_case()) {
        case proto::Type::kBoolType: {
            Layout layout;
            layout.size = 1;
            layout.alignment = 1;
            return layout;
        }
        case proto::Type::kIntegerType: {
            Layout layout;
            layout.size = static_cast<std::size_t>(type.integer_type().width()) / 8;
            layout.alignment = layout.size;
            return layout;
        }
        case proto::Type::kFloatType: {
            Layout layout;
            layout.size = static_cast<std::size_t>(type.float_type().precision()) / 8;
            layout.alignment = layout.size;
            return layout;
        }
        case proto::Type::kStringType: {
            // Variable-length: no fixed size/alignment to report. Only valid
            // as a top-level Value's type; struct/array cases below reject
            // it (directly or via a type_ref that resolves to it) rather
            // than silently treating it as a zero-size field.
            Layout layout;
            layout.isVariableLength = true;
            return layout;
        }
        case proto::Type::kStructType: {
            // Standard C struct layout: each field goes at the next offset
            // that's a multiple of its own alignment, and the whole struct's
            // size is padded up to a multiple of its largest member's
            // alignment (so arrays of this struct also stay aligned).
            Layout layout;
            std::size_t offset = 0;
            std::size_t maxAlign = 1;
            for (auto const& field : type.struct_type().fields()) {
                Layout fieldLayout = computeLayout(field.base_type(), registry);
                if (fieldLayout.isVariableLength) {
                    throw std::invalid_argument(
                        "computeLayout: struct field '" + field.name() +
                        "' is variable-length (e.g. a string), which cannot be nested in a struct");
                }
                offset = roundUp(offset, fieldLayout.alignment);
                layout.fields.push_back(FieldLayout{field.name(), offset, field.base_type()});
                offset += fieldLayout.size;
                maxAlign = std::max(maxAlign, fieldLayout.alignment);
            }
            layout.alignment = maxAlign;
            layout.size = roundUp(offset, maxAlign);
            return layout;
        }
        case proto::Type::kArrayType: {
            Layout elementLayout = computeLayout(type.array_type().base_type(), registry);
            if (elementLayout.isVariableLength) {
                throw std::invalid_argument(
                    "computeLayout: an array of variable-length elements (e.g. strings) is not supported");
            }
            std::size_t count = 1;
            for (auto dimension : type.array_type().dimensions()) {
                count *= dimension;
            }
            Layout layout;
            // elementLayout.size is already padded to elementLayout.alignment
            // (structs are padded to their own alignment above, and scalars
            // are always self-aligned), so it doubles as the array stride.
            layout.elementStride = elementLayout.size;
            layout.elementCount = count;
            layout.alignment = elementLayout.alignment;
            layout.size = layout.elementStride * count;
            return layout;
        }
        case proto::Type::kTypeRef:
            return registry.getLayout(type.type_ref());
        case proto::Type::TYPE_NOT_SET:
        default:
            throw std::invalid_argument("computeLayout: Type has no case set");
    }
}

} // frelsim::type::core
