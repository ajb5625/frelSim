#pragma once
#include <set>

/**
 * \file StableId.hpp
 * \brief StableId is a monotonically increasing unique Id.
 */

namespace frelsim::util {

class StableId final {

    public:

        StableId() = default;

        ~StableId() = default;

        /**
         * \brief Get a new unique Id.
         */
        std::size_t getUniqueId();

        /**
         * \brief Remove an Id from use.
         * \param id The unique ID to reclaim.
         */
        void removeUniqueId(std::size_t id);

    private:
        /// @brief The monotonically increasing id number.
        std::size_t currentId_ = 0;

        /// @brief Ids to be reused.
        std::set<std::size_t> reclaimedIds_;

};

} // frelsim::util