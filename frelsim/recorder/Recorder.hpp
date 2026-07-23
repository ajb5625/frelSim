#pragma once

#include <functional>
#include <string>
#include <vector>
#include "../util/Aliases.hpp"
#include "../util/Identifier.hpp"

namespace frelsim::recorder {

/**
 * \file Recorder.hpp
 * \brief Accumulates a numeric time series for a fixed set of watched
 * identifiers - one row per call to record(), one column per watched
 * identifier - and writes it out as CSV for offline verification/plotting
 * (see scripts/plot_results.py). This is the piece that answers "how do we
 * check a simulation's results are actually correct" - point-in-time
 * inspection via Overseer::get() can't show a trajectory, only a snapshot.
 *
 * Deliberately CSV/file-based rather than a live streaming connection: no
 * gRPC service exists yet to stream over (see docs/PROGRESS.md, tasks #3/
 * #5). The design keeps that door open, though - record() takes a getter
 * callback rather than reaching into a Simulator directly, so the same
 * Recorder could be driven by a future streaming handler instead of
 * Simulator's step observer, without changing Recorder itself.
 */
class Recorder {
    public:
        using Getter = std::function<Values(std::string const& domain, Identifiers const& ids)>;

        /**
         * \brief \param watched Identifiers to sample on every record() call
         * (domain = a composed Simulation's key, scope/name = the specific
         * field - same convention as RoutedSimulation wiring). Every watched
         * identifier must resolve to a numeric Value (FloatType/
         * IntegerType/BoolType) - a string/struct/array can't be plotted as
         * a time series and isn't supported here.
         */
        explicit Recorder(std::vector<util::Identifier> watched);

        /**
         * \brief Samples every watched identifier's current value (grouped
         * by domain, one getter call per domain) and appends one row.
         * \throws std::logic_error if a watched identifier resolves to a
         * non-numeric Value.
         */
        void record(double time, Getter const& getter);

        /**
         * \brief Writes every recorded row to path as CSV: a header row
         * (`time` followed by each watched identifier as `domain.scope.name`),
         * then one row per record() call, in the order they were recorded.
         * \throws std::invalid_argument if path can't be opened for writing.
         */
        void writeCsv(std::string const& path) const;

        /// \brief Number of rows recorded so far (debug/test use).
        std::size_t rowCount() const;

    private:
        std::vector<util::Identifier> watched_;

        /// \brief times_[i] is the time of row i; rows_[i] holds one value
        /// per watched_ entry for that same row.
        std::vector<double> times_;
        std::vector<std::vector<double>> rows_;
};

} // namespace frelsim::recorder
