#pragma once
#include <memory>
#include <string>
#include <vector>

#include "frelsim/proto/Simulation.pb.h"

namespace frelsim::task {

/**
 * \file Task.hpp
 * \brief Task is the metadata associated with a simulation. 
 * This is mainly centered around sample time and when a task begins.
 * Additionally, contains which type of task it is.
 */

/**
 * \brief TaskType tells if the task is 
 * Continuous
 * PeriodicDiscrete (Called at a periodic rate)
 * AperiodicDiscrete (An event which fires only once)
 */
enum class TaskType : int {
    Continuous = 0, 
    PeriodicDiscrete = 1,
    AperiodicDiscrete = 2
};

using TaskId = std::size_t;



class Task final {

    public:
        /**
         * \brief Task constructor.
         * \param taskId The numeric Id of the task.
         * \param period The frequency at which the task is executed.
         * \param offset The time at which the task begins.
         */
        Task(TaskId taskId, double period = 0.0, double offset = 0.0, double maxStepSize = -1.0);

        ~Task() = default;

        TaskId taskId() const;

        TaskType taskType() const;

        double period() const;

        double offset() const;

        double maxStepSize() const;

    private:
        /// @brief  The string representation of the task.
        const TaskId taskId_;

        /// @brief The frequency at which the task is executed.
        const double period_;

        /// @brief The time at which the task begins.
        const double offset_;

        /// @brief Longest time to go without an integration step.
        double maxStepSize_ = -1.0;

        /// @brief The type of task.
        TaskType taskType_;


        // std::vector<std::unique_ptr<Task>> subtasks_;

};

frelsim::sim::proto::TaskType serializeTaskTypeEnum(const TaskType cppType);

frelsim::sim::proto::Task serialize(Task cppTask);

std::unique_ptr<Task> deserialize(frelsim::sim::proto::Task protoTask);

} // frelsim::task