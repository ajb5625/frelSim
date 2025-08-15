#pragma once

#include "../task/Task.hpp"

namespace frelsim::schedule {

using TaskList = std::vector<std::unique_ptr<task::Task>>;

class Scheduler final {

    public:
        Scheduler(TaskList& taskList);

        ~Scheduler() = default;

        std::vector<std::string> taskHits(double simulationTime);

    private:

        void sortTasks(TaskList& taskList); 

        std::vector<std::string> continuousTaskIds_; // These always have hits, so cache them

        TaskList continuousTasks_;

        TaskList periodicDiscreteTasks_;

        TaskList aperiodicDiscreteTasks_;




};
}
