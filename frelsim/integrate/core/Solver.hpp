#include <functional>

namespace frelsim::integrate::core {

class Solver {
    
    public:
        Solver();

        virtual ~Solver();

        virtual bool step(double stopTime) = 0;

};


} // namespace frelsim::integrate::core