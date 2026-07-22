#include <gtest/gtest.h>
#include "frelsim/type/core/Value.hpp"
#include "frelsim/type/core/TypeRegistry.hpp"

namespace frelsim::type::core {
namespace {

TEST(ValueTest, MakeIntRoundTripsPositiveAndNegative) {
    EXPECT_EQ(Value::makeInt(42, 32).asInt(), 42);
    EXPECT_EQ(Value::makeInt(-42, 32).asInt(), -42);
    EXPECT_EQ(Value::makeInt(-1, 8).asInt(), -1);
    EXPECT_EQ(Value::makeInt(127, 8).asInt(), 127);
}

TEST(ValueTest, MakeIntStoresExactlyWidthOverEightBytes) {
    EXPECT_EQ(Value::makeInt(1, 8).getBytes().size(), 1u);
    EXPECT_EQ(Value::makeInt(1, 16).getBytes().size(), 2u);
    EXPECT_EQ(Value::makeInt(1, 32).getBytes().size(), 4u);
    EXPECT_EQ(Value::makeInt(1, 64).getBytes().size(), 8u);
}

TEST(ValueTest, MakeIntEncodesLittleEndian) {
    // 0x0102 as a 16-bit int should store the low byte first.
    Value value = Value::makeInt(0x0102, 16);
    ASSERT_EQ(value.getBytes().size(), 2u);
    EXPECT_EQ(value.getBytes()[0], std::byte{0x02});
    EXPECT_EQ(value.getBytes()[1], std::byte{0x01});
}

TEST(ValueTest, MakeUnsignedIntDoesNotSignExtend) {
    Value value = Value::makeInt(255, 8, /*isSigned=*/false);
    EXPECT_EQ(value.asInt(), 255);
}

TEST(ValueTest, MakeBoolRoundTrips) {
    EXPECT_TRUE(Value::makeBool(true).asBool());
    EXPECT_FALSE(Value::makeBool(false).asBool());
    EXPECT_EQ(Value::makeBool(true).getBytes().size(), 1u);
}

TEST(ValueTest, MakeDoubleRoundTripsAtBothPrecisions) {
    EXPECT_DOUBLE_EQ(Value::makeDouble(3.5, 64).asDouble(), 3.5);
    EXPECT_EQ(Value::makeDouble(3.5, 64).getBytes().size(), 8u);

    // Narrowing to float32 loses precision, so compare loosely.
    EXPECT_NEAR(Value::makeDouble(3.5, 32).asDouble(), 3.5, 1e-6);
    EXPECT_EQ(Value::makeDouble(3.5, 32).getBytes().size(), 4u);
}

TEST(ValueTest, MakeStringRoundTrips) {
    Value value = Value::makeString("frelsim");
    EXPECT_EQ(value.asString(), "frelsim");
    EXPECT_EQ(value.getBytes().size(), 7u);
}

TEST(ValueTest, AsIntThrowsIfTypeIsNotInteger) {
    EXPECT_THROW(Value::makeBool(true).asInt(), std::logic_error);
}

TEST(ValueTest, StructFieldAccessReadsAndWritesAtComputedOffsets) {
    TypeRegistry registry;
    proto::Type structType;
    auto* fields = structType.mutable_struct_type();
    {
        auto* f = fields->add_fields();
        f->set_name("height");
        f->mutable_base_type()->mutable_float_type()->set_precision(64);
    }
    {
        auto* f = fields->add_fields();
        f->set_name("bounces");
        f->mutable_base_type()->mutable_integer_type()->set_width(32);
        f->mutable_base_type()->mutable_integer_type()->set_is_signed(true);
    }
    registry.registerType("frelsim.type.BallState", structType);

    proto::Type ref;
    ref.set_type_ref("frelsim.type.BallState");
    Value ball(ref, std::vector<std::byte>(registry.getLayout("frelsim.type.BallState").size));

    ball.setField("height", Value::makeDouble(9.8), registry);
    ball.setField("bounces", Value::makeInt(3, 32), registry);

    EXPECT_DOUBLE_EQ(ball.getField("height", registry).asDouble(), 9.8);
    EXPECT_EQ(ball.getField("bounces", registry).asInt(), 3);
}

TEST(ValueTest, StructFieldAccessThrowsOnUnknownFieldName) {
    TypeRegistry registry;
    proto::Type structType;
    auto* field = structType.mutable_struct_type()->add_fields();
    field->set_name("x");
    field->mutable_base_type()->mutable_float_type()->set_precision(64);

    Value value(structType, std::vector<std::byte>(8));
    EXPECT_THROW(value.getField("y", registry), std::out_of_range);
}

TEST(ValueTest, SetFieldRejectsWrongSizedValue) {
    TypeRegistry registry;
    proto::Type structType;
    auto* field = structType.mutable_struct_type()->add_fields();
    field->set_name("x");
    field->mutable_base_type()->mutable_float_type()->set_precision(64);

    Value value(structType, std::vector<std::byte>(8));
    EXPECT_THROW(value.setField("x", Value::makeInt(1, 32), registry), std::invalid_argument);
}

TEST(ValueTest, ArrayElementAccessReadsAndWritesByIndex) {
    TypeRegistry registry;
    proto::Type arrayType;
    arrayType.mutable_array_type()->mutable_base_type()->mutable_integer_type()->set_width(32);
    arrayType.mutable_array_type()->mutable_base_type()->mutable_integer_type()->set_is_signed(true);
    arrayType.mutable_array_type()->add_dimensions(3);

    Value array(arrayType, std::vector<std::byte>(12, std::byte{0}));
    EXPECT_EQ(array.arrayLength(registry), 3u);

    array.setElement(0, Value::makeInt(10, 32), registry);
    array.setElement(1, Value::makeInt(20, 32), registry);
    array.setElement(2, Value::makeInt(30, 32), registry);

    EXPECT_EQ(array.getElement(0, registry).asInt(), 10);
    EXPECT_EQ(array.getElement(1, registry).asInt(), 20);
    EXPECT_EQ(array.getElement(2, registry).asInt(), 30);
}

TEST(ValueTest, ArrayElementAccessThrowsOnOutOfRangeIndex) {
    TypeRegistry registry;
    proto::Type arrayType;
    arrayType.mutable_array_type()->mutable_base_type()->mutable_float_type()->set_precision(64);
    arrayType.mutable_array_type()->add_dimensions(2);

    Value array(arrayType, std::vector<std::byte>(16, std::byte{0}));
    EXPECT_THROW(array.getElement(2, registry), std::out_of_range);
    EXPECT_THROW(array.setElement(2, Value::makeDouble(1.0), registry), std::out_of_range);
}

TEST(ValueTest, ArrayOfStructsNestsFieldAccessThroughElementAccess) {
    TypeRegistry registry;
    proto::Type pointType;
    {
        auto* fields = pointType.mutable_struct_type();
        auto* x = fields->add_fields();
        x->set_name("x");
        x->mutable_base_type()->mutable_float_type()->set_precision(64);
        auto* y = fields->add_fields();
        y->set_name("y");
        y->mutable_base_type()->mutable_float_type()->set_precision(64);
    }
    registry.registerType("frelsim.type.Point", pointType);

    proto::Type arrayType;
    arrayType.mutable_array_type()->mutable_base_type()->set_type_ref("frelsim.type.Point");
    arrayType.mutable_array_type()->add_dimensions(2);

    Layout const& arrayLayout = registry.registerType("frelsim.type.PointArray", arrayType);
    proto::Type arrayRef;
    arrayRef.set_type_ref("frelsim.type.PointArray");
    Value points(arrayRef, std::vector<std::byte>(arrayLayout.size, std::byte{0}));

    Value first = points.getElement(0, registry);
    first.setField("x", Value::makeDouble(1.0), registry);
    first.setField("y", Value::makeDouble(2.0), registry);
    points.setElement(0, first, registry);

    Value readBack = points.getElement(0, registry);
    EXPECT_DOUBLE_EQ(readBack.getField("x", registry).asDouble(), 1.0);
    EXPECT_DOUBLE_EQ(readBack.getField("y", registry).asDouble(), 2.0);
}

} // namespace
} // namespace frelsim::type::core
