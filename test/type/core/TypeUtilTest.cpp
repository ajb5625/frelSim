#include <gtest/gtest.h>
#include "frelsim/type/core/TypeUtil.hpp"

namespace frelsim::type::core {
namespace {

proto::Type makeFloat(int precision) {
    proto::Type type;
    type.mutable_float_type()->set_precision(precision);
    return type;
}

proto::Type makeInt(bool isSigned, int width) {
    proto::Type type;
    type.mutable_integer_type()->set_is_signed(isSigned);
    type.mutable_integer_type()->set_width(width);
    return type;
}

proto::Type makeBool() {
    proto::Type type;
    type.mutable_bool_type();
    return type;
}

proto::Type makeString() {
    proto::Type type;
    type.mutable_string_type();
    return type;
}

proto::Type makeTypeRef(std::string const& uri) {
    proto::Type type;
    type.set_type_ref(uri);
    return type;
}

proto::Type makeStruct(std::vector<std::pair<std::string, proto::Type>> const& fields) {
    proto::Type type;
    for (auto const& [name, fieldType] : fields) {
        auto* field = type.mutable_struct_type()->add_fields();
        field->set_name(name);
        *field->mutable_base_type() = fieldType;
    }
    return type;
}

proto::Type makeArray(proto::Type const& base, std::vector<std::uint32_t> const& dims) {
    proto::Type type;
    *type.mutable_array_type()->mutable_base_type() = base;
    for (auto dim : dims) {
        type.mutable_array_type()->add_dimensions(dim);
    }
    return type;
}

TEST(TypeUtilTest, DifferentOneofCasesAreNeverEqual) {
    EXPECT_FALSE(typesEqual(makeFloat(64), makeInt(true, 64)));
    EXPECT_FALSE(typesEqual(makeBool(), makeString()));
}

TEST(TypeUtilTest, FloatTypesEqualOnlyWithMatchingPrecision) {
    EXPECT_TRUE(typesEqual(makeFloat(64), makeFloat(64)));
    EXPECT_FALSE(typesEqual(makeFloat(64), makeFloat(32)));
}

TEST(TypeUtilTest, IntegerTypesEqualOnlyWithMatchingSignednessAndWidth) {
    EXPECT_TRUE(typesEqual(makeInt(true, 32), makeInt(true, 32)));
    EXPECT_FALSE(typesEqual(makeInt(true, 32), makeInt(false, 32)));
    EXPECT_FALSE(typesEqual(makeInt(true, 32), makeInt(true, 64)));
}

TEST(TypeUtilTest, BoolAndStringTypesAreAlwaysEqualToThemselves) {
    EXPECT_TRUE(typesEqual(makeBool(), makeBool()));
    EXPECT_TRUE(typesEqual(makeString(), makeString()));
}

TEST(TypeUtilTest, TypeRefsCompareByUriOnly) {
    EXPECT_TRUE(typesEqual(makeTypeRef("myDomain.myScope.MyStruct"), makeTypeRef("myDomain.myScope.MyStruct")));
    EXPECT_FALSE(typesEqual(makeTypeRef("myDomain.myScope.MyStruct"), makeTypeRef("myDomain.myScope.OtherStruct")));
}

TEST(TypeUtilTest, TypeRefIsNotResolvedAgainstAnEquivalentInlineDescription) {
    // Deliberate limitation, documented on typesEqual itself: no
    // TypeRegistry is available to resolve a type_ref against, so this
    // compares unequal even though both sides describe the same FloatType.
    EXPECT_FALSE(typesEqual(makeTypeRef("myDomain.myScope.MyFloat"), makeFloat(64)));
}

TEST(TypeUtilTest, StructTypesRequireSameFieldNamesTypesAndOrder) {
    proto::Type const a = makeStruct({{"x", makeFloat(64)}, {"y", makeFloat(64)}});
    proto::Type const b = makeStruct({{"x", makeFloat(64)}, {"y", makeFloat(64)}});
    proto::Type const differentName = makeStruct({{"x", makeFloat(64)}, {"z", makeFloat(64)}});
    proto::Type const differentType = makeStruct({{"x", makeFloat(64)}, {"y", makeInt(true, 32)}});
    proto::Type const differentOrder = makeStruct({{"y", makeFloat(64)}, {"x", makeFloat(64)}});
    proto::Type const differentFieldCount = makeStruct({{"x", makeFloat(64)}});

    EXPECT_TRUE(typesEqual(a, b));
    EXPECT_FALSE(typesEqual(a, differentName));
    EXPECT_FALSE(typesEqual(a, differentType));
    EXPECT_FALSE(typesEqual(a, differentOrder));
    EXPECT_FALSE(typesEqual(a, differentFieldCount));
}

TEST(TypeUtilTest, ArrayTypesRequireSameElementTypeAndDimensions) {
    proto::Type const a = makeArray(makeFloat(64), {3, 3});
    proto::Type const b = makeArray(makeFloat(64), {3, 3});
    proto::Type const differentDims = makeArray(makeFloat(64), {3, 4});
    proto::Type const differentElementType = makeArray(makeInt(true, 32), {3, 3});

    EXPECT_TRUE(typesEqual(a, b));
    EXPECT_FALSE(typesEqual(a, differentDims));
    EXPECT_FALSE(typesEqual(a, differentElementType));
}

TEST(TypeUtilTest, AnUnsetTypeIsNeverEqualToAnything) {
    proto::Type unset;
    EXPECT_FALSE(typesEqual(unset, unset));
    EXPECT_FALSE(typesEqual(unset, makeFloat(64)));
}

} // namespace
} // namespace frelsim::type::core
