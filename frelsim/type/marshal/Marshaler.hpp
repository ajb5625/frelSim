#pragma once
#include "frelsim/proto/Value.pb.h"
#include "../core/Value.hpp"

namespace frelsim::type::marshal {

// proto::Value and core::Value are now both just {Type, bytes} - no
// recursion needed here (struct/array field interpretation happens later,
// via core::Value::getField/getElement against a TypeRegistry), just a
// bytes <-> std::string transcode.
class Marshaler final {

    public:
        Marshaler() = default;

        ~Marshaler() = default;

        core::Value protoToCpp(const proto::Value& protoValue) const;

        proto::Value cppToProto(const core::Value& cppValue) const;

};


} // frelsim::type::marshal
