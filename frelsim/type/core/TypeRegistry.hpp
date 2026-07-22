#pragma once
#include <map>
#include <string>
#include "Layout.hpp"
#include "frelsim/proto/Type.pb.h"

namespace frelsim::type::core {

/**
 * \brief Registers named schemas (Types), keyed by a URI using the same
 * domain.scope.name convention as Identifier, and caches each one's computed
 * Layout so it's paid for once at registration rather than recomputed on
 * every Value that references it via type_ref.
 *
 * Scoped to a System composition, so multiple components can register/refer
 * to the same struct schema and actually agree on a wire-compatible layout
 * for exchanging structured data - see docs/PROGRESS.md, type system track.
 */
class TypeRegistry final {

    public:
        TypeRegistry() = default;

        /**
         * \brief Register `type` under `uri`, computing and caching its
         * Layout (resolving any type_ref it contains against this registry).
         * \throws std::invalid_argument if `uri` is already registered, or
         * if `type` fails to lay out (see computeLayout()).
         */
        Layout const& registerType(std::string const& uri, proto::Type type);

        /**
         * \brief The Type previously registered under `uri`.
         * \throws std::out_of_range if `uri` is not registered.
         */
        proto::Type const& getType(std::string const& uri) const;

        /**
         * \brief The Layout computed when `uri` was registered.
         * \throws std::out_of_range if `uri` is not registered.
         */
        Layout const& getLayout(std::string const& uri) const;

        bool has(std::string const& uri) const;

    private:
        struct Entry {
            proto::Type type;
            Layout layout;
        };

        std::map<std::string, Entry> entries_;

};

} // frelsim::type::core
