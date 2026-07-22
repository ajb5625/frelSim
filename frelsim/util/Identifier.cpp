#include "Identifier.hpp"
#include <ranges>
#include <stdexcept>
#include <vector>

namespace frelsim::util {

Identifier::Identifier(const std::string& uri) : uri_(uri){
    // Split on '.' into a vector first rather than a fixed-size array: a
    // malformed URI (too few or too many dot-separated parts) must be
    // rejected explicitly here, not silently overrun a fixed buffer.
    std::vector<std::string> idParts;
    for (auto token : uri | std::views::split('.')) {
        idParts.emplace_back(token.begin(), token.end());
    }
    if (idParts.size() != 3) {
        throw std::invalid_argument("Identifier: expected \"domain.scope.name\", got \"" + uri + "\"");
    }
    domain_ = std::move(idParts[0]);
    scope_ = std::move(idParts[1]);
    name_ = std::move(idParts[2]);
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