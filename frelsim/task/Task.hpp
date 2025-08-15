#pragma once
#include <memory>
#include <string>
#include <vector>

namespace frelsim::task {

enum class TaskType : int {
    Continuous = 0, 
    PeriodicDiscrete = 1,
    AperiodicDiscrete = 2
};


class Task final {

    public:
        Task(const std::string& taskId, double period = 0.0, double offset = 0.0);

        ~Task() = default;

        const std::string& taskId() const;

        TaskType taskType() const;

        double period() const;

        double offset() const;

    private:

        const std::string taskId_;

        const double period_;

        const double offset_;

        TaskType taskType_;


        // std::vector<std::unique_ptr<Task>> subtasks_;

};


} // frelsim::task