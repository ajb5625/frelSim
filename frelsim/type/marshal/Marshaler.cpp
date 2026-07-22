#include "Marshaler.hpp"
#include <algorithm>

namespace frelsim::type::marshal {

core::Value Marshaler::protoToCpp(const proto::Value& protoValue) const {
    std::string const& wireBytes = protoValue.data();
    std::vector<std::byte> bytes(wireBytes.size());
    std::transform(wireBytes.begin(), wireBytes.end(), bytes.begin(),
                    [](char c) { return static_cast<std::byte>(c); });
    return core::Value(protoValue.type(), std::move(bytes));
}

proto::Value Marshaler::cppToProto(const core::Value& cppValue) const {
    proto::Value protoValue;
    *protoValue.mutable_type() = cppValue.getType();
    auto const& bytes = cppValue.getBytes();
    protoValue.mutable_data()->assign(reinterpret_cast<char const*>(bytes.data()), bytes.size());
    return protoValue;
}

} // frelsim::type::marshal
