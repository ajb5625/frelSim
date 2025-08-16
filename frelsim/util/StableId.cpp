#include "StableId.hpp"

namespace frelsim::util {

std::size_t StableId::getUniqueId() {
    if (reclaimedIds_.empty()) {
        return currentId_++;
    }
    auto beginning = reclaimedIds_.cbegin();
    std::size_t id = *beginning;
    reclaimedIds_.erase(beginning);
    return id;
}

void StableId::removeUniqueId(std::size_t id) {
    reclaimedIds_.insert(id);
}



} // frelsim::util