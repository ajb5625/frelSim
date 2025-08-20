#include "Task.hpp"
#include "../util/almost_equal.hpp"

namespace frelsim::task {

frelsim::sim::proto::TaskType serializeTaskTypeEnum(const TaskType cppType) {
    using fsptt = frelsim::sim::proto::TaskType;
    switch (cppType) {
        case TaskType::Continuous:
            return fsptt::Continuous;
        break;
        case TaskType::PeriodicDiscrete:
            return fsptt::PeriodicDiscrete;
        break;
        case TaskType::AperiodicDiscrete:
            return fsptt::AperiodicDiscrete;
        break;
    }
}

frelsim::sim::proto::Task serialize(Task cppTask) {
    frelsim::sim::proto::Task protoTask;
    protoTask.set_task_id(cppTask.taskId());
    protoTask.set_period(cppTask.period());
    protoTask.set_offset(cppTask.offset());
    protoTask.set_max_step_size(cppTask.maxStepSize());
    protoTask.set_task_type(serializeTaskTypeEnum(cppTask.taskType()));
    return protoTask;
}

std::unique_ptr<Task> deserialize(frelsim::sim::proto::Task protoTask) {
    double maxStepSize = -1.0;
    if (protoTask.has_max_step_size()) {
        maxStepSize = protoTask.max_step_size();
    }
    return std::make_unique<Task>(protoTask.task_id()
                , protoTask.period()
                , protoTask.offset()
                , maxStepSize);
;
}

Task::Task(TaskId taskId
        , double period
        , double offset
        , double maxStepSize) : taskId_(taskId)
                            , period_(period)
                            , offset_(offset)
                            , maxStepSize_(maxStepSize) {

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

TaskId Task::taskId() const {
    return taskId_;
}

TaskType Task::taskType() const {
    return taskType_;
}

double Task::period() const {
    return period_;
}

double Task::offset() const {
    return offset_;
}

double Task::maxStepSize() const {
    return maxStepSize_;
}

} // frelsim::task
