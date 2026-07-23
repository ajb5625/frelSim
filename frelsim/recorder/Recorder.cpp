#include "Recorder.hpp"
#include <fstream>
#include <iomanip>
#include <map>
#include <stdexcept>

namespace frelsim::recorder {

namespace {

// util::Identifier::getUri() only reflects a URI actually passed to its
// string constructor - every identifier here is built via setDomain/
// setScope/setName instead, so getUri() would just be empty. Format
// domain.scope.name directly instead, matching the convention used
// throughout Linker/Simulator.
std::string describeIdentifier(util::Identifier const& id) {
    return id.getDomain() + "." + id.getScope() + "." + id.getName();
}

double toDouble(type::core::Value const& value, util::Identifier const& id) {
    auto const& type = value.getType();
    switch (type.type_case()) {
        case type::proto::Type::kFloatType:
            return value.asDouble();
        case type::proto::Type::kIntegerType:
            return static_cast<double>(value.asInt());
        case type::proto::Type::kBoolType:
            return value.asBool() ? 1.0 : 0.0;
        default:
            throw std::logic_error(
                "Recorder: watched identifier '" + describeIdentifier(id) +
                "' is not a numeric type - only Float/Integer/Bool values can be recorded as a time series");
    }
}

} // namespace

Recorder::Recorder(std::vector<util::Identifier> watched) : watched_(std::move(watched)) {}

void Recorder::record(double time, Getter const& getter) {
    std::vector<double> row(watched_.size(), 0.0);

    // Group by domain so each distinct Simulation is only asked once per
    // record() call, even if several watched identifiers belong to it.
    std::map<std::string, std::vector<std::size_t>> indicesByDomain;
    for (std::size_t i = 0; i < watched_.size(); ++i) {
        indicesByDomain[watched_[i].getDomain()].push_back(i);
    }

    for (auto const& [domain, indices] : indicesByDomain) {
        Identifiers ids;
        ids.reserve(indices.size());
        for (auto const idx : indices) {
            ids.push_back(watched_[idx]);
        }

        Values const values = getter(domain, ids);
        if (values.size() != ids.size()) {
            throw std::logic_error(
                "Recorder: getter for domain '" + domain + "' returned " + std::to_string(values.size()) +
                " values for " + std::to_string(ids.size()) + " requested identifiers");
        }

        for (std::size_t j = 0; j < indices.size(); ++j) {
            row[indices[j]] = toDouble(values[j], watched_[indices[j]]);
        }
    }

    times_.push_back(time);
    rows_.push_back(std::move(row));
}

void Recorder::writeCsv(std::string const& path) const {
    std::ofstream file(path);
    if (!file) {
        throw std::invalid_argument("Recorder: could not open '" + path + "' for writing");
    }

    file << "time";
    for (auto const& id : watched_) {
        file << "," << describeIdentifier(id);
    }
    file << "\n";

    file << std::setprecision(10);
    for (std::size_t i = 0; i < times_.size(); ++i) {
        file << times_[i];
        for (double const value : rows_[i]) {
            file << "," << value;
        }
        file << "\n";
    }
}

std::size_t Recorder::rowCount() const {
    return times_.size();
}

} // namespace frelsim::recorder
