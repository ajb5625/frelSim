#include "TypeUtil.hpp"
#include <algorithm>

namespace frelsim::type::core {

namespace {

bool structTypesEqual(proto::StructType const& a, proto::StructType const& b) {
    if (a.fields_size() != b.fields_size()) {
        return false;
    }
    return std::equal(a.fields().begin(), a.fields().end(), b.fields().begin(),
        [](proto::StructFieldType const& fa, proto::StructFieldType const& fb) {
            return fa.name() == fb.name() && typesEqual(fa.base_type(), fb.base_type());
        });
}

bool arrayTypesEqual(proto::ArrayType const& a, proto::ArrayType const& b) {
    if (!typesEqual(a.base_type(), b.base_type())) {
        return false;
    }
    return std::equal(a.dimensions().begin(), a.dimensions().end(), b.dimensions().begin(), b.dimensions().end());
}

} // namespace

bool typesEqual(proto::Type const& a, proto::Type const& b) {
    if (a.type_case() != b.type_case()) {
        return false;
    }

    switch (a.type_case()) {
        case proto::Type::kIntegerType:
            return a.integer_type().is_signed() == b.integer_type().is_signed()
                && a.integer_type().width() == b.integer_type().width();
        case proto::Type::kFloatType:
            return a.float_type().precision() == b.float_type().precision();
        case proto::Type::kBoolType:
            return true;
        case proto::Type::kStringType:
            return true;
        case proto::Type::kStructType:
            return structTypesEqual(a.struct_type(), b.struct_type());
        case proto::Type::kArrayType:
            return arrayTypesEqual(a.array_type(), b.array_type());
        case proto::Type::kTypeRef:
            return a.type_ref() == b.type_ref();
        case proto::Type::TYPE_NOT_SET:
        default:
            return false;
    }
}

} // namespace frelsim::type::core
