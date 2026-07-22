#include <gtest/gtest.h>
#include "frelsim/type/core/Layout.hpp"
#include "frelsim/type/core/TypeRegistry.hpp"

namespace frelsim::type::core {
namespace {

proto::Type boolType() {
    proto::Type t;
    t.mutable_bool_type();
    return t;
}

proto::Type intType(int width, bool isSigned = true) {
    proto::Type t;
    t.mutable_integer_type()->set_width(width);
    t.mutable_integer_type()->set_is_signed(isSigned);
    return t;
}

proto::Type floatType(int precision) {
    proto::Type t;
    t.mutable_float_type()->set_precision(precision);
    return t;
}

proto::Type stringType() {
    proto::Type t;
    t.mutable_string_type();
    return t;
}

TEST(LayoutTest, BoolIsOneByteAligned) {
    TypeRegistry registry;
    Layout layout = computeLayout(boolType(), registry);
    EXPECT_EQ(layout.size, 1u);
    EXPECT_EQ(layout.alignment, 1u);
}

TEST(LayoutTest, IntegerSizeAndAlignmentMatchWidth) {
    TypeRegistry registry;
    for (int width : {8, 16, 32, 64}) {
        Layout layout = computeLayout(intType(width), registry);
        EXPECT_EQ(layout.size, static_cast<std::size_t>(width) / 8) << "width=" << width;
        EXPECT_EQ(layout.alignment, static_cast<std::size_t>(width) / 8) << "width=" << width;
    }
}

TEST(LayoutTest, FloatSizeAndAlignmentMatchPrecision) {
    TypeRegistry registry;
    Layout f32 = computeLayout(floatType(32), registry);
    EXPECT_EQ(f32.size, 4u);
    EXPECT_EQ(f32.alignment, 4u);

    Layout f64 = computeLayout(floatType(64), registry);
    EXPECT_EQ(f64.size, 8u);
    EXPECT_EQ(f64.alignment, 8u);
}

TEST(LayoutTest, StringIsVariableLength) {
    TypeRegistry registry;
    Layout layout = computeLayout(stringType(), registry);
    EXPECT_TRUE(layout.isVariableLength);
}

// struct { bool a; int32 b; double c; } - the textbook C struct-packing
// example: `a` takes 1 byte but `b` needs 4-byte alignment, so 3 bytes of
// padding go between them; `c` needs 8-byte alignment and is already at
// offset 8, so no padding there; final size is padded to the struct's own
// (8-byte, from the double) alignment, which it already satisfies.
TEST(LayoutTest, StructFieldsGetCStylePaddingAndAlignment) {
    TypeRegistry registry;
    proto::Type structType;
    auto* fields = structType.mutable_struct_type();
    {
        auto* f = fields->add_fields();
        f->set_name("a");
        *f->mutable_base_type() = boolType();
    }
    {
        auto* f = fields->add_fields();
        f->set_name("b");
        *f->mutable_base_type() = intType(32);
    }
    {
        auto* f = fields->add_fields();
        f->set_name("c");
        *f->mutable_base_type() = floatType(64);
    }

    Layout layout = computeLayout(structType, registry);
    ASSERT_EQ(layout.fields.size(), 3u);
    EXPECT_EQ(layout.fields[0].name, "a");
    EXPECT_EQ(layout.fields[0].offset, 0u);
    EXPECT_EQ(layout.fields[1].name, "b");
    EXPECT_EQ(layout.fields[1].offset, 4u);
    EXPECT_EQ(layout.fields[2].name, "c");
    EXPECT_EQ(layout.fields[2].offset, 8u);
    EXPECT_EQ(layout.alignment, 8u);
    EXPECT_EQ(layout.size, 16u);
}

TEST(LayoutTest, StructRejectsNestedStringField) {
    TypeRegistry registry;
    proto::Type structType;
    auto* field = structType.mutable_struct_type()->add_fields();
    field->set_name("label");
    *field->mutable_base_type() = stringType();

    EXPECT_THROW(computeLayout(structType, registry), std::invalid_argument);
}

TEST(LayoutTest, ArrayStrideAndSizeMatchElementAndDimensions) {
    TypeRegistry registry;
    proto::Type arrayType;
    *arrayType.mutable_array_type()->mutable_base_type() = intType(32);
    arrayType.mutable_array_type()->add_dimensions(3);

    Layout layout = computeLayout(arrayType, registry);
    EXPECT_EQ(layout.elementStride, 4u);
    EXPECT_EQ(layout.elementCount, 3u);
    EXPECT_EQ(layout.size, 12u);
    EXPECT_EQ(layout.alignment, 4u);
}

TEST(LayoutTest, ArrayOfMultipleDimensionsMultipliesElementCount) {
    TypeRegistry registry;
    proto::Type arrayType;
    *arrayType.mutable_array_type()->mutable_base_type() = floatType(64);
    arrayType.mutable_array_type()->add_dimensions(3);
    arrayType.mutable_array_type()->add_dimensions(3);

    Layout layout = computeLayout(arrayType, registry);
    EXPECT_EQ(layout.elementCount, 9u);
    EXPECT_EQ(layout.size, 9u * 8u);
}

TEST(LayoutTest, ArrayRejectsStringElements) {
    TypeRegistry registry;
    proto::Type arrayType;
    *arrayType.mutable_array_type()->mutable_base_type() = stringType();
    arrayType.mutable_array_type()->add_dimensions(2);

    EXPECT_THROW(computeLayout(arrayType, registry), std::invalid_argument);
}

TEST(LayoutTest, TypeRefResolvesThroughRegistry) {
    TypeRegistry registry;
    registry.registerType("frelsim.type.Point", [] {
        proto::Type structType;
        auto* fields = structType.mutable_struct_type();
        auto* x = fields->add_fields();
        x->set_name("x");
        *x->mutable_base_type() = floatType(64);
        auto* y = fields->add_fields();
        y->set_name("y");
        *y->mutable_base_type() = floatType(64);
        return structType;
    }());

    proto::Type ref;
    ref.set_type_ref("frelsim.type.Point");

    Layout layout = computeLayout(ref, registry);
    EXPECT_EQ(layout.size, 16u);
    ASSERT_EQ(layout.fields.size(), 2u);
    EXPECT_EQ(layout.fields[0].name, "x");
    EXPECT_EQ(layout.fields[1].name, "y");
}

TEST(LayoutTest, UnsetTypeThrows) {
    TypeRegistry registry;
    proto::Type unset;
    EXPECT_THROW(computeLayout(unset, registry), std::invalid_argument);
}

} // namespace
} // namespace frelsim::type::core
