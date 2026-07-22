#include <gtest/gtest.h>
#include <stdexcept>
#include "frelsim/util/Identifier.hpp"

namespace frelsim::util {
namespace {

TEST(IdentifierTest, ParsesDomainScopeName) {
    Identifier id("sim.Output.height");
    EXPECT_EQ(id.getDomain(), "sim");
    EXPECT_EQ(id.getScope(), "Output");
    EXPECT_EQ(id.getName(), "height");
    EXPECT_EQ(id.getUri(), "sim.Output.height");
}

TEST(IdentifierTest, SettersOverrideParsedParts) {
    Identifier id("sim.Output.height");
    id.setDomain("other");
    id.setScope("Parameter");
    id.setName("gravity");
    EXPECT_EQ(id.getDomain(), "other");
    EXPECT_EQ(id.getScope(), "Parameter");
    EXPECT_EQ(id.getName(), "gravity");
}

TEST(IdentifierTest, RejectsUriWithTooFewParts) {
    EXPECT_THROW(Identifier("sim.Output"), std::invalid_argument);
}

TEST(IdentifierTest, RejectsUriWithTooManyParts) {
    EXPECT_THROW(Identifier("sim.Output.height.extra"), std::invalid_argument);
}

TEST(IdentifierTest, DefaultConstructedHasEmptyParts) {
    Identifier id;
    EXPECT_EQ(id.getDomain(), "");
    EXPECT_EQ(id.getScope(), "");
    EXPECT_EQ(id.getName(), "");
}

} // namespace
} // namespace frelsim::util
