#include "Value.hpp"
#include "Layout.hpp"
#include "TypeRegistry.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace frelsim::type::core {

namespace {

std::vector<std::byte> encodeUnsignedLE(std::uint64_t value, std::size_t numBytes) {
    std::vector<std::byte> bytes(numBytes);
    for (std::size_t i = 0; i < numBytes; ++i) {
        bytes[i] = static_cast<std::byte>((value >> (8 * i)) & 0xFFu);
    }
    return bytes;
}

std::uint64_t decodeUnsignedLE(std::vector<std::byte> const& bytes) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        value |= (static_cast<std::uint64_t>(bytes[i]) << (8 * i));
    }
    return value;
}

// bytes is known to hold exactly numBytes bytes of a two's-complement signed
// integer; extend its sign bit out to the full 64 bits.
std::int64_t signExtend(std::uint64_t value, std::size_t numBytes) {
    std::size_t const bits = numBytes * 8;
    if (bits == 0 || bits >= 64) {
        return static_cast<std::int64_t>(value);
    }
    std::uint64_t const signBit = std::uint64_t{1} << (bits - 1);
    if (value & signBit) {
        value |= (~std::uint64_t{0} << bits);
    }
    return static_cast<std::int64_t>(value);
}

// getType() is commonly a type_ref (see Type.proto) rather than an inline
// description; struct/array field access needs the concrete, resolved Type
// to know what it's actually looking at.
proto::Type const& resolveType(proto::Type const& type, TypeRegistry const& registry) {
    if (type.type_case() == proto::Type::kTypeRef) {
        return registry.getType(type.type_ref());
    }
    return type;
}

} // namespace

Value::Value(proto::Type type, std::vector<std::byte> bytes) : type_(std::move(type)), bytes_(std::move(bytes)) {}

proto::Type const& Value::getType() const {
    return type_;
}

std::vector<std::byte> const& Value::getBytes() const {
    return bytes_;
}

void Value::setBytes(std::vector<std::byte> bytes) {
    bytes_ = std::move(bytes);
}

void Value::setType(proto::Type type) {
    type_ = std::move(type);
}

Value Value::makeInt(std::int64_t value, int width, bool isSigned) {
    proto::Type type;
    auto* integerType = type.mutable_integer_type();
    integerType->set_width(width);
    integerType->set_is_signed(isSigned);
    std::size_t const numBytes = static_cast<std::size_t>(width) / 8;
    return Value(std::move(type), encodeUnsignedLE(static_cast<std::uint64_t>(value), numBytes));
}

Value Value::makeBool(bool value) {
    proto::Type type;
    type.mutable_bool_type();
    return Value(std::move(type), std::vector<std::byte>{static_cast<std::byte>(value ? 1 : 0)});
}

Value Value::makeDouble(double value, int precision) {
    proto::Type type;
    type.mutable_float_type()->set_precision(precision);
    if (precision == 32) {
        float const narrowed = static_cast<float>(value);
        std::uint32_t const bits = std::bit_cast<std::uint32_t>(narrowed);
        return Value(std::move(type), encodeUnsignedLE(bits, 4));
    }
    std::uint64_t const bits = std::bit_cast<std::uint64_t>(value);
    return Value(std::move(type), encodeUnsignedLE(bits, 8));
}

Value Value::makeString(std::string value) {
    proto::Type type;
    type.mutable_string_type();
    std::vector<std::byte> bytes(value.size());
    std::memcpy(bytes.data(), value.data(), value.size());
    return Value(std::move(type), std::move(bytes));
}

std::int64_t Value::asInt() const {
    if (!type_.has_integer_type()) {
        throw std::logic_error("Value::asInt: type is not IntegerType");
    }
    std::uint64_t const raw = decodeUnsignedLE(bytes_);
    if (type_.integer_type().is_signed()) {
        return signExtend(raw, bytes_.size());
    }
    return static_cast<std::int64_t>(raw);
}

bool Value::asBool() const {
    if (!type_.has_bool_type()) {
        throw std::logic_error("Value::asBool: type is not BoolType");
    }
    if (bytes_.empty()) {
        throw std::logic_error("Value::asBool: no data");
    }
    return bytes_[0] != std::byte{0};
}

double Value::asDouble() const {
    if (!type_.has_float_type()) {
        throw std::logic_error("Value::asDouble: type is not FloatType");
    }
    std::uint64_t const raw = decodeUnsignedLE(bytes_);
    if (type_.float_type().precision() == 32) {
        return static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(raw)));
    }
    return std::bit_cast<double>(raw);
}

std::string Value::asString() const {
    if (!type_.has_string_type()) {
        throw std::logic_error("Value::asString: type is not StringType");
    }
    return std::string(reinterpret_cast<char const*>(bytes_.data()), bytes_.size());
}

Value Value::getField(std::string const& fieldName, TypeRegistry const& registry) const {
    Layout const layout = computeLayout(type_, registry);
    for (auto const& field : layout.fields) {
        if (field.name != fieldName) {
            continue;
        }
        Layout const fieldLayout = computeLayout(field.type, registry);
        if (bytes_.size() < field.offset + fieldLayout.size) {
            throw std::out_of_range("Value::getField: '" + fieldName + "' extends past the end of this Value's bytes");
        }
        std::vector<std::byte> fieldBytes(
            bytes_.begin() + static_cast<std::ptrdiff_t>(field.offset),
            bytes_.begin() + static_cast<std::ptrdiff_t>(field.offset + fieldLayout.size));
        return Value(field.type, std::move(fieldBytes));
    }
    throw std::out_of_range("Value::getField: no field named '" + fieldName + "'");
}

void Value::setField(std::string const& fieldName, Value const& fieldValue, TypeRegistry const& registry) {
    Layout const layout = computeLayout(type_, registry);
    for (auto const& field : layout.fields) {
        if (field.name != fieldName) {
            continue;
        }
        Layout const fieldLayout = computeLayout(field.type, registry);
        if (fieldValue.getBytes().size() != fieldLayout.size) {
            throw std::invalid_argument(
                "Value::setField: '" + fieldName + "' expects " + std::to_string(fieldLayout.size) +
                " bytes, got " + std::to_string(fieldValue.getBytes().size()));
        }
        if (bytes_.size() < field.offset + fieldLayout.size) {
            bytes_.resize(field.offset + fieldLayout.size, std::byte{0});
        }
        std::copy(fieldValue.getBytes().begin(), fieldValue.getBytes().end(),
                  bytes_.begin() + static_cast<std::ptrdiff_t>(field.offset));
        return;
    }
    throw std::out_of_range("Value::setField: no field named '" + fieldName + "'");
}

Value Value::getElement(std::size_t index, TypeRegistry const& registry) const {
    Layout const layout = computeLayout(type_, registry);
    if (index >= layout.elementCount) {
        throw std::out_of_range("Value::getElement: index out of range");
    }
    proto::Type const& elementType = resolveType(type_, registry).array_type().base_type();
    Layout const elementLayout = computeLayout(elementType, registry);
    std::size_t const offset = index * layout.elementStride;
    if (bytes_.size() < offset + elementLayout.size) {
        throw std::out_of_range("Value::getElement: element extends past the end of this Value's bytes");
    }
    std::vector<std::byte> elementBytes(
        bytes_.begin() + static_cast<std::ptrdiff_t>(offset),
        bytes_.begin() + static_cast<std::ptrdiff_t>(offset + elementLayout.size));
    return Value(elementType, std::move(elementBytes));
}

void Value::setElement(std::size_t index, Value const& elementValue, TypeRegistry const& registry) {
    Layout const layout = computeLayout(type_, registry);
    if (index >= layout.elementCount) {
        throw std::out_of_range("Value::setElement: index out of range");
    }
    if (elementValue.getBytes().size() != layout.elementStride) {
        throw std::invalid_argument("Value::setElement: element size mismatch");
    }
    std::size_t const offset = index * layout.elementStride;
    if (bytes_.size() < offset + layout.elementStride) {
        bytes_.resize(offset + layout.elementStride, std::byte{0});
    }
    std::copy(elementValue.getBytes().begin(), elementValue.getBytes().end(),
              bytes_.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::size_t Value::arrayLength(TypeRegistry const& registry) const {
    return computeLayout(type_, registry).elementCount;
}

} // frelsim::type::core
