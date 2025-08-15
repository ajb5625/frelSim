#include "Scheduler.hpp"
#include "../util/almost_equal.hpp"

namespace frelsim::schedule {

Scheduler::Scheduler(TaskList& taskList) {
    sortTasks(taskList);
}


void Scheduler::sortTasks(TaskList& taskList) {
    for (auto& task : taskList) {
        switch(task->taskType()) {
            case task::TaskType::Continuous:
                continuousTaskIds_.push_back(task->taskId());
                continuousTasks_.push_back(std::move(task));
            break;
            case task::TaskType::PeriodicDiscrete:
                periodicDiscreteTasks_.push_back(std::move(task));
            break;
            case task::TaskType::AperiodicDiscrete:
                aperiodicDiscreteTasks_.push_back(std::move(task));
            break;

        }
    }
}

std::vector<std::string> Scheduler::taskHits(double simulationTime) {
    std::vector<std::string> taskHitVector = continuousTaskIds_;
    for (const auto& aperiodicDiscreteTask : aperiodicDiscreteTasks_) {
        if (util::almostEqual(simulationTime - aperiodicDiscreteTask->offset(), 0.0)) {
            taskHitVector.push_back(aperiodicDiscreteTask->taskId());
        }
    }
    for (const auto& periodicDiscreteTask : periodicDiscreteTasks_) {
        double simTimeNoOffset = simulationTime - periodicDiscreteTask->offset();
        if (simTimeNoOffset >= 0.0 && util::almostEqual(std::fmod(simTimeNoOffset, periodicDiscreteTask->period()), 0.0)) {
            taskHitVector.push_back(periodicDiscreteTask->taskId())   ;
        }
    }
    return taskHitVector;
}


} // frelsim::schedule