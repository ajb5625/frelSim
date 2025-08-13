#include "../core/Solver.hpp"

namespace frelsim::integrate::expl {


class Euler : public core::Solver {

    public:
        Euler(double tFinal);

        ~Euler() override;

        bool step(double stopTime) override;

    private:
        const double tFinal_;


};


} // namespace frelsim::integrate::explicit