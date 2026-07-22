#include <gtest/gtest.h>
#include "frelsim/type/marshal/Marshaler.hpp"
#include "frelsim/type/core/TypeRegistry.hpp"

namespace frelsim::type::marshal {
namespace {

using core::Value;

TEST(MarshalerTest, RoundTripsDoublePreservingExactBytes) {
    Marshaler marshaler;
    Value original = Value::makeDouble(2.71828);
    Value roundTripped = marshaler.protoToCpp(marshaler.cppToProto(original));

    EXPECT_EQ(roundTripped.getBytes(), original.getBytes());
    EXPECT_DOUBLE_EQ(roundTripped.asDouble(), 2.71828);
}

TEST(MarshalerTest, RoundTripsIntPreservingExactBytes) {
    Marshaler marshaler;
    Value original = Value::makeInt(-1234, 32);
    Value roundTripped = marshaler.protoToCpp(marshaler.cppToProto(original));

    EXPECT_EQ(roundTripped.getBytes(), original.getBytes());
    EXPECT_EQ(roundTripped.asInt(), -1234);
}

TEST(MarshalerTest, RoundTripsStringPreservingExactBytes) {
    Marshaler marshaler;
    Value original = Value::makeString("frelsim");
    Value roundTripped = marshaler.protoToCpp(marshaler.cppToProto(original));

    EXPECT_EQ(roundTripped.asString(), "frelsim");
}

TEST(MarshalerTest, RoundTripsEmptyBytes) {
    Marshaler marshaler;
    Value original; // default-constructed: unset type, empty bytes
    Value roundTripped = marshaler.protoToCpp(marshaler.cppToProto(original));

    EXPECT_TRUE(roundTripped.getBytes().empty());
}

TEST(MarshalerTest, RoundTripsStructBytesSoFieldAccessStillWorksAfter) {
    core::TypeRegistry registry;
    proto::Type structType;
    auto* field = structType.mutable_struct_type()->add_fields();
    field->set_name("height");
    field->mutable_base_type()->mutable_float_type()->set_precision(64);
    core::Layout const& layout = registry.registerType("frelsim.type.RoundTripStruct", structType);

    proto::Type ref;
    ref.set_type_ref("frelsim.type.RoundTripStruct");
    Value original(ref, std::vector<std::byte>(layout.size, std::byte{0}));
    original.setField("height", Value::makeDouble(9.8), registry);

    Marshaler marshaler;
    Value roundTripped = marshaler.protoToCpp(marshaler.cppToProto(original));

    EXPECT_DOUBLE_EQ(roundTripped.getField("height", registry).asDouble(), 9.8);
}

} // namespace
} // namespace frelsim::type::marshal
