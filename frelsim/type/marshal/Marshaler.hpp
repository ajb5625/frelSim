#include "frelsim/type/proto/Value.pb.h"
#include "../core/Value.hpp"

namespace frelsim::type::marshal {

class Marshaler final {

    public:
        Marshaler() = default;

        ~Marshaler() = default;

        core::Value protoToCpp(const proto::Value& protoValue);

        proto::Value cppToProto(const core::Value& cppValue);
        

};


} // frelsim::type::marshal