#include <gtest/gtest.h>
#include <stdexcept>
#include "frelsim/task/Task.hpp"

namespace frelsim::task {
namespace {

TEST(TaskTest, ZeroPeriodAndOffsetIsContinuous) {
    Task task(0, /*period=*/0.0, /*offset=*/0.0);
    EXPECT_EQ(task.taskType(), TaskType::Continuous);
}

TEST(TaskTest, ZeroPeriodWithNonzeroOffsetIsAperiodicDiscrete) {
    Task task(0, /*period=*/0.0, /*offset=*/2.5);
    EXPECT_EQ(task.taskType(), TaskType::AperiodicDiscrete);
}

TEST(TaskTest, NonzeroPeriodIsPeriodicDiscrete) {
    Task task(0, /*period=*/0.1, /*offset=*/0.0);
    EXPECT_EQ(task.taskType(), TaskType::PeriodicDiscrete);

    Task taskWithOffset(0, /*period=*/0.1, /*offset=*/0.05);
    EXPECT_EQ(taskWithOffset.taskType(), TaskType::PeriodicDiscrete);
}

TEST(TaskTest, AccessorsReturnConstructorValues) {
    Task task(/*taskId=*/7, /*period=*/0.1, /*offset=*/0.05, /*maxStepSize=*/0.01);
    EXPECT_EQ(task.taskId(), 7u);
    EXPECT_DOUBLE_EQ(task.period(), 0.1);
    EXPECT_DOUBLE_EQ(task.offset(), 0.05);
    EXPECT_DOUBLE_EQ(task.maxStepSize(), 0.01);
}

TEST(TaskTest, SerializeTaskTypeEnumMapsAllKnownValues) {
    EXPECT_EQ(serializeTaskTypeEnum(TaskType::Continuous), frelsim::sim::proto::TaskType::Continuous);
    EXPECT_EQ(serializeTaskTypeEnum(TaskType::PeriodicDiscrete), frelsim::sim::proto::TaskType::PeriodicDiscrete);
    EXPECT_EQ(serializeTaskTypeEnum(TaskType::AperiodicDiscrete), frelsim::sim::proto::TaskType::AperiodicDiscrete);
}

TEST(TaskTest, SerializeTaskTypeEnumThrowsOnUnrecognizedValue) {
    EXPECT_THROW(serializeTaskTypeEnum(static_cast<TaskType>(99)), std::invalid_argument);
}

TEST(TaskTest, SerializeDeserializeRoundTripsAllFields) {
    Task original(/*taskId=*/3, /*period=*/0.2, /*offset=*/0.1, /*maxStepSize=*/0.02);

    frelsim::sim::proto::Task proto = serialize(original);
    EXPECT_EQ(proto.task_id(), 3);
    EXPECT_DOUBLE_EQ(proto.period(), 0.2);
    EXPECT_DOUBLE_EQ(proto.offset(), 0.1);
    EXPECT_DOUBLE_EQ(proto.max_step_size(), 0.02);
    EXPECT_EQ(proto.task_type(), frelsim::sim::proto::TaskType::PeriodicDiscrete);

    std::unique_ptr<Task> roundTripped = deserialize(proto);
    EXPECT_EQ(roundTripped->taskId(), original.taskId());
    EXPECT_DOUBLE_EQ(roundTripped->period(), original.period());
    EXPECT_DOUBLE_EQ(roundTripped->offset(), original.offset());
    EXPECT_DOUBLE_EQ(roundTripped->maxStepSize(), original.maxStepSize());
    EXPECT_EQ(roundTripped->taskType(), original.taskType());
}

TEST(TaskTest, DeserializeWithoutMaxStepSizeDefaultsToNegativeOne) {
    frelsim::sim::proto::Task proto;
    proto.set_task_id(1);
    proto.set_period(0.0);
    proto.set_offset(0.0);
    // max_step_size deliberately left unset (it's an `optional` field).

    std::unique_ptr<Task> task = deserialize(proto);
    EXPECT_DOUBLE_EQ(task->maxStepSize(), -1.0);
}

} // namespace
} // namespace frelsim::task
