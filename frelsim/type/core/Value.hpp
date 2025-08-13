#include <variant>
#include "frelsim/type/proto/Type.pb.h"

namespace frelsim::type::core {

using Data = std::variant<std::monostate, double>;


class Value final {
    public:

        Value() = default;

        Value(const type::proto::Type& type, const Data& data);

        ~Value() = default;

        const type::proto::Type& getType() const;

        const Data& getData() const;

        void setData(const Data& data);

        void setType(const type::proto::Type& type);

    private:
        type::proto::Type type_;

        Data data_;




};

}


