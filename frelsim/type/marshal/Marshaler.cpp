#include "Marshaler.hpp"


namespace frelsim::type::marshal {

core::Value Marshaler::protoToCpp(const proto::Value& protoValue) {
    core::Value cppValue;
    auto type = protoValue.type();
    if (type.has_float_type()) {
        core::Data payload = protoValue.double_value();
        cppValue.setData(payload);
        cppValue.setType(protoValue.type());
    }
    return cppValue;
}

proto::Value cppToProto(const core::Value& cppValue) {
    proto::Value protoValue;
    auto type = cppValue.getType();
    if (type.has_float_type()) {
        core::Data payload = cppValue.getData();
        protoValue.set_double_value(std::get<double>(payload));
    }
    return protoValue;
}


} // frelsim::type::marshal