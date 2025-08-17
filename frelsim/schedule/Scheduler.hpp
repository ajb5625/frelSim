#pragma once

#include "../task/Task.hpp"

namespace frelsim::schedule {

/**
 * \file Scheduler.hpp
 * \brief The scheduler maintains and provides utilities for Tasks and their associated sample times.
 */

using TaskPtr = std::unique_ptr<task::Task>;
using TaskList = std::vector<TaskPtr>;

class Scheduler final {

    public:
        /**
         * \brief Scheduler constructor.
         * \param taskList The list of tasks the scheduler maintains.
         */
        Scheduler(TaskList& taskList);

        ~Scheduler() = default;

        /**
         * \brief Given a simulationTime, return the list of tasks that must be executed at the simulationTime.
         * \param simulationTime The current global simulation time.
         */
        std::vector<task::TaskId> taskHits(double simulationTime) const;

        /**
         * \brief Tells if aperiodic tasks are upcoming.
         */
        bool areAperiodicTasksUpcoming() const;

        /**
         * \brief Get the ID and time of the next aperiodic discrete task.
         * \returns Pair of task id, time of event.
         */
        std::pair<task::TaskId, double> getNextAperiodicTask() const;

        /**
         * \brief Tell the scheduler what time it is at the bottom of the simulation step.
         * This will remove aperiodic discrete tasks with offset >= simulationTime.
         * \param simulationTime The current global simulation time from the simulator.
         */
        void reconcileScheduler(double simulationTime);

        /**
         * \brief The scheduler will obtain the next time of a periodic or aperiodic discrete
         * task.
         */
        double getNextDiscreteTime(double currentTime) const;

    private:

        /**
         * \brief Sort the tasks into their task types.
         * This will also put the aperiodic discrete tasks in order by time.
         * \param taskList The list of unsorted tasks.
         */
        void sortTasks(TaskList& taskList); 

        double time_ = 0.0;

        std::vector<task::TaskId> continuousTaskIds_; // These always have hits, so cache them

        TaskList continuousTasks_;

        TaskList periodicDiscreteTasks_;

        TaskList aperiodicDiscreteTasks_;




};
}
