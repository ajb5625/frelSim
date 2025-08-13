#include <memory>
#include <functional>

namespace frelsim {

using State = std::vector<double>;

using Derivative = std::shared_ptr<std::function<State(const State&, double)>>;

using Matrix = std::vector<State>;

} // frelsim::util