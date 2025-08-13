#include "Value.hpp"

namespace frelsim::type::core {

Value::Value(const type::proto::Type& type, const Data& data) : type_(type), data_(data) {}

const type::proto::Type& Value::getType() const {
    return type_;
}

const Data& Value::getData() const { 
    return data_;
}

void Value::setData(const Data& data) {
    data_ = data;
}

void Value::setType(const type::proto::Type& type) {
    type_ = type;
}


} // frelsim::type::core