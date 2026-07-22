#include <gtest/gtest.h>
#include "frelsim/type/core/TypeRegistry.hpp"

namespace frelsim::type::core {
namespace {

proto::Type makeVector3Type() {
    proto::Type arrayType;
    auto* array = arrayType.mutable_array_type();
    array->mutable_base_type()->mutable_float_type()->set_precision(64);
    array->add_dimensions(3);
    return arrayType;
}

TEST(TypeRegistryTest, HasIsFalseBeforeRegistration) {
    TypeRegistry registry;
    EXPECT_FALSE(registry.has("frelsim.type.Vector3"));
}

TEST(TypeRegistryTest, RegisterThenLookupReturnsSameType) {
    TypeRegistry registry;
    registry.registerType("frelsim.type.Vector3", makeVector3Type());

    EXPECT_TRUE(registry.has("frelsim.type.Vector3"));
    proto::Type const& type = registry.getType("frelsim.type.Vector3");
    EXPECT_TRUE(type.has_array_type());
}

TEST(TypeRegistryTest, RegisterCachesComputedLayout) {
    TypeRegistry registry;
    Layout const& layout = registry.registerType("frelsim.type.Vector3", makeVector3Type());
    EXPECT_EQ(layout.elementCount, 3u);
    EXPECT_EQ(layout.size, 24u);

    // getLayout should return the exact same computed values, not recompute
    // something different.
    Layout const& fetched = registry.getLayout("frelsim.type.Vector3");
    EXPECT_EQ(fetched.size, layout.size);
    EXPECT_EQ(fetched.elementCount, layout.elementCount);
}

TEST(TypeRegistryTest, RegisteringDuplicateNameThrows) {
    TypeRegistry registry;
    registry.registerType("frelsim.type.Vector3", makeVector3Type());
    EXPECT_THROW(registry.registerType("frelsim.type.Vector3", makeVector3Type()), std::invalid_argument);
}

TEST(TypeRegistryTest, LookupOfUnregisteredNameThrows) {
    TypeRegistry registry;
    EXPECT_THROW(registry.getType("frelsim.type.NoSuchType"), std::out_of_range);
    EXPECT_THROW(registry.getLayout("frelsim.type.NoSuchType"), std::out_of_range);
}

TEST(TypeRegistryTest, RegisteringAnInvalidTypeDoesNotLeaveAPartialEntry) {
    TypeRegistry registry;
    proto::Type structWithStringField;
    auto* field = structWithStringField.mutable_struct_type()->add_fields();
    field->set_name("label");
    field->mutable_base_type()->mutable_string_type();

    EXPECT_THROW(registry.registerType("frelsim.type.Bad", structWithStringField), std::invalid_argument);
    // Since layout computation failed, the name must not be registered at all.
    EXPECT_FALSE(registry.has("frelsim.type.Bad"));
}

} // namespace
} // namespace frelsim::type::core
