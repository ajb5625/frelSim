#include "Scheduler.hpp"
#include <algorithm>
#include "../util/Aliases.hpp"
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
    std::sort(aperiodicDiscreteTasks_.begin()
            , aperiodicDiscreteTasks_.end()
            , [](const TaskPtr& a, const TaskPtr& b) {
                return a->offset() < b->offset();
            });
}

std::vector<task::TaskId> Scheduler::taskHits(double simulationTime) const {
    std::vector<task::TaskId> taskHitVector = continuousTaskIds_;
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

std::pair<task::TaskId, double> Scheduler::getNextAperiodicTask() const {
    auto nextADTask = aperiodicDiscreteTasks_[0].get();
    return std::pair(nextADTask->taskId(), nextADTask->offset());
}

bool Scheduler::areAperiodicTasksUpcoming() const {
    return !aperiodicDiscreteTasks_.empty();
}

void Scheduler::reconcileScheduler(double simulationTime) {
    time_ = simulationTime;
    std::erase_if(aperiodicDiscreteTasks_, [simulationTime](const TaskPtr& task){
        return task->offset() <= simulationTime;
    });
}

// TODO review this.
double Scheduler::getNextDiscreteTime(double currentTime) const {
    if (currentTime < 0.0) {
        return 0.0;
    }
    double nextTime = std::numeric_limits<double>::max();
    if (areAperiodicTasksUpcoming()) {
        nextTime = getNextAperiodicTask().second;
    }
    for (const auto& discreteTask: periodicDiscreteTasks_) {
        double candidateTime = std::numeric_limits<double>::max();
        double offset = discreteTask->offset();
        double period = discreteTask->period();
        double firesIn = offset - currentTime;
        if (firesIn >= TinyTolerance) {
            // Task hasn't fired yet
            candidateTime = offset;
        }
        else {
            // Task has already fired and will fire periodically
            // We've passed (or are at) the first offset; compute the next hit
            const double periodsElapsed = (currentTime - offset) / period;
            const double nextIndex = std::ceil(periodsElapsed - TinyTolerance);
            candidateTime = offset + nextIndex * period;
        }

        nextTime = std::min(nextTime, candidateTime);
    }
    return nextTime;
}


} // frelsim::schedule