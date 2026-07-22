#include "TypeRegistry.hpp"
#include <stdexcept>

namespace frelsim::type::core {

Layout const& TypeRegistry::registerType(std::string const& uri, proto::Type type) {
    if (has(uri)) {
        throw std::invalid_argument("TypeRegistry: '" + uri + "' is already registered");
    }
    // computeLayout runs (and can throw) before the entry is inserted, so a
    // failed registration doesn't leave a partially-registered name behind.
    Layout layout = computeLayout(type, *this);
    Entry& entry = entries_[uri];
    entry.type = std::move(type);
    entry.layout = std::move(layout);
    return entry.layout;
}

proto::Type const& TypeRegistry::getType(std::string const& uri) const {
    return entries_.at(uri).type;
}

Layout const& TypeRegistry::getLayout(std::string const& uri) const {
    return entries_.at(uri).layout;
}

bool TypeRegistry::has(std::string const& uri) const {
    return entries_.find(uri) != entries_.end();
}

} // frelsim::type::core
