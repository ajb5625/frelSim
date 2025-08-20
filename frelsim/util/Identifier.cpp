#include "Identifier.hpp"
#include <ranges>

namespace frelsim::util {

Identifier::Identifier(const std::string& uri) : uri_(uri){
    std::array<std::string, 3> idParts;
    std::size_t idx = 0;
    for (auto token : uri | std::views::split('.')) {
        idParts[idx++] = std::string(token.begin(), token.end());
    }
    domain_ = idParts[0];
    scope_ = idParts[1];
    name_ = idParts[2];
}

const std::string& Identifier::getUri() const {
    return uri_;
}

void Identifier::setDomain(const std::string& domain) {
    domain_ = domain;
}

void Identifier::setScope(const std::string& scope) {
    scope_ = scope;
}

void Identifier::setName(const std::string& name) {
    name_ = name;
}

const std::string& Identifier::getDomain() const {
    return domain_;
}

const std::string& Identifier::getScope() const {
    return scope_;
}

const std::string& Identifier::getName() const {
    return name_;
}
} // frelsim::util