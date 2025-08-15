#include "Task.hpp"
#include "../util/almost_equal.hpp"

namespace frelsim::task {

Task::Task(const std::string& taskId
        , double period
        , double offset) : taskId_(taskId)
                            , period_(period)
                            , offset_(offset) {

    if (util::almostEqual(period_, 0.0) && util::almostEqual(offset_, 0.0)) {
        taskType_ = TaskType::Continuous;
    }
    else if (!util::almostEqual(offset_, 0.0) && util::almostEqual(period_, 0.0)) {
        taskType_ = TaskType::AperiodicDiscrete;
    }
    else {
        taskType_ = TaskType::PeriodicDiscrete;
    }
}

const std::string& Task::taskId() const {
    return taskId_;
}

TaskType Task::taskType() const {
    return taskType_;
}

double Task::period() const {
    return period_;
}

} // frelsim::task
