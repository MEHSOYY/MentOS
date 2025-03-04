/// @file scheduler_algorithm.c
/// @brief Round Robin algorithm.
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

// Setup the logging for this file (do this before any other include).
#include "sys/kernel_levels.h"           // Include kernel log levels.
#define __DEBUG_HEADER__ "[SCHALG]"      ///< Change header.
#define __DEBUG_LEVEL__  LOGLEVEL_NOTICE ///< Set log level.
#include "io/debug.h"                    // Include debugging functions.

#include "assert.h"
#include "hardware/timer.h"
#include "list_head.h"
#include "process/prio.h"
#include "process/scheduler.h"
#include "process/scheduler_feedback.h"
#include "process/wait.h"

/// @brief Updates task execution statistics.
/// @param task the task to update.
static void __update_task_statistics(task_struct *task);

/// @brief Checks if the given task is actually a periodic task.
/// @param task the task to check.
/// @return true if the task is periodic, false otherwise.
static inline bool_t __is_periodic_task(task_struct *task)
{
    // Check if the task is a periodic one and it is not under analysis.
    return task->se.is_periodic && !task->se.is_under_analysis;
}

/// @brief Employs time-sharing, giving each job a time-slot, and is also
/// preemptive since the scheduler forces the task out of the CPU once
/// the time-slot expires.
/// @param runqueue list of all processes.
/// @param skip_periodic tells the algorithm that periodic processes in the list
/// should be skipped.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_rr(runqueue_t *runqueue, bool_t skip_periodic)
{
    // If there is just one task, return it; no need to do anything.
    if (list_head_size(&runqueue->curr->run_list) <= 1) {
        return runqueue->curr;
    }
    // This will hold a given entry, while iterating the list of tasks.
    task_struct *entry = NULL;
    // Search for the next task (we do not start from the head, so INSIDE, skip the head).
    list_for_each_decl (it, &runqueue->curr->run_list) {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue) {
            continue;
        }
        // Get the current entry.
        entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes.
        if (entry->state != TASK_RUNNING) {
            continue;
        }
        // If entry is a periodic task, and we were asked to skip periodic tasks, skip it.
        if (__is_periodic_task(entry) && skip_periodic) {
            continue;
        }
        // We have our next entry.
        return entry;
    }

    return NULL;
}

/// @brief Is a non-preemptive algorithm, where each task is assigned a
/// priority. Processes with highest priority are executed first, while
/// processes with same priority are executed on first-come/first-served basis.
/// Priority can be decided based on memory requirements, time requirements or
/// any other resource requirement.
/// @param runqueue list of all processes.
/// @param skip_periodic tells the algorithm if there are periodic processes in
/// the list, and in that case it needs to skip them.
/// @return the next task on success, NULL on failure.
/// @details
/// When implementing this algorithm, beware of the following pitfal. If you
/// have the following runqueue (reports task position in the runqueue, priority
/// and name):
///      Position | Priority | Name
///          1    |    120   | init
///          2    |    120   | shell
///          3    |    122   | echo
///          4    |    128   | ps
/// If you pick the first task every time (i.e., init), and use its prio (i.e.,
/// 120), what would happen if inside the for-loop when you check "if the entry
/// has a lower priority", you use a lesser-than sign?
/// First, it will check against init itself, so 120 < 120 is false.
/// Then, it will check against shell, again, 120 < 120 is false.
/// As such, shell or the other processes will never be selected. There are
/// different ways of solving this problem, each of which requires changes only
/// inside this same function. Good luck.
static inline task_struct *__scheduler_priority(runqueue_t *runqueue, bool_t skip_periodic)
{
#ifdef SCHEDULER_PRIORITY
    // Get the first element of the list.
    task_struct *next  = list_entry(runqueue->queue.next, struct task_struct, run_list);
    // This will hold a given entry, while iterating the list of tasks.
    task_struct *entry = NULL;
    // Get the  static priority of the first element of the list we just extracted.
    time_t min         = /*...*/;

    // Search for the task with the smallest static priority.
    list_for_each_decl (it, &runqueue->queue) {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is a periodic task, and we were asked to skip periodic tasks, skip it.
        if (__is_periodic_task(entry) && skip_periodic)
            continue;
        // Check if the entry has a lower priority.
        if (/*...*/) {
            // Chose the `entry` as the `next` task.
            /*...*/
        }
    }
    return next;
#else
    return __scheduler_rr(runqueue, skip_periodic);
#endif
}

/// @brief It aims at giving a fair share of CPU time to processes, and achieves
/// that by associating a virtual runtime to each of them. It always tries to
/// run the task with the smallest vruntime (i.e., the task which executed least
/// so far). It always tries to split up CPU time between runnable tasks as
/// close to "ideal multitasking hardware" as possible.
/// @param runqueue list of all processes.
/// @param skip_periodic tells the algorithm if there are periodic processes in
/// the list, and in that case it needs to skip them.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_cfs(runqueue_t *runqueue, bool_t skip_periodic)
{
#ifdef SCHEDULER_CFS
    // Get the first element of the list.
    task_struct *next  = list_entry(runqueue->queue.next, struct task_struct, run_list);
    // This will hold a given entry, while iterating the list of tasks.
    task_struct *entry = NULL;
    // Get the virtual runtime of the first element of the list we just extracted.
    time_t min         = /*...*/;

    // Search for the task with the smallest vruntime value.
    list_for_each_decl (it, &runqueue->queue) {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is a periodic task, and we were asked to skip periodic tasks, skip it.
        if (__is_periodic_task(entry) && skip_periodic)
            continue;

        // Check if the element in the list has a smaller vruntime value.
        /* ... */
    }

    return next;
#else
    return __scheduler_rr(runqueue, skip_periodic);
#endif
}

/// @brief Executes the task with the earliest absolute deadline among all the
/// ready tasks.
/// @param runqueue list of all processes.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_aedf(runqueue_t *runqueue)
{
#ifdef SCHEDULER_AEDF
    // This will hold the pointer to the next task to schedule.
    task_struct *next  = list_entry(runqueue->queue.next, struct task_struct, run_list);
    // This will hold a given entry, while iterating the list of tasks.
    task_struct *entry = NULL;
    // Initialize the nearest "next deadline".
    time_t min         = UINT_MAX;

    // Iter over the runqueue to find the task with the earliest absolute deadline.
    list_for_each_decl (it, &runqueue->queue) {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is not a periodic task, skip it.
        if (!__is_periodic_task(entry))
            continue;
        // If the entry has passed its deadline, we output a warning but it
        // might still be selected as next task.
        if (/*...*/) {
            pr_warning(
                "Process %d passed its deadline %d < %d \n", entry->pid,
                /*...*/,
                /*...*/);
        }
        // Now, select the task if it has the minimum absolute deadline.
        if (/*...*/ < /*...*/) {
            // Set it as next task.
            next = entry;
            // Update the minimum absolute deadline.
            min  = /*...*/;
        }
    }
    if (next) {
        pr_debug(
            "[%9d] Activating task '%16s', deadline: %5d \t\n", next->pid, next->name,
            /*...*/);
        return next;
    }
#endif
    // If there are no tasks with deadline != 0 to execute, just pick another
    // task by using Round-Robin.
    return __scheduler_rr(runqueue, false);
}

/// @brief Executes the task with the earliest absolute DEADLINE among all the
/// ready tasks. When a task was executed, and its period is starting again, it
/// must be set as 'executable again', and its deadline and next_period must be
/// updated.
/// @param runqueue list of all processes.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_edf(runqueue_t *runqueue)
{
#ifdef SCHEDULER_EDF
    // This will hold the pointer to the next task to schedule.
    task_struct *next  = NULL;
    // This will hold a given entry, while iterating the list of tasks.
    task_struct *entry = NULL;
    // Initialize the nearest "next deadline".
    time_t min         = UINT_MAX;

    // Iter over the runqueue to find the task with the earliest absolute deadline.
    list_for_each_decl (it, &runqueue->queue) {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is not a periodic task, skip it.
        if (!__is_periodic_task(entry))
            continue;

        if (entry->se.executed) {
            // If the period for the entry is starting again and it has already
            //  executed, set it as 'executable again'. Deadline and next_period
            //  are propagated.
            if (/*...*/ <= /*...*/) {
                entry->se.executed = /*...*/;
                entry->se.deadline += /*...*/;
                entry->se.next_period += /*...*/;
                pr_debug(
                    "[%9d] Activating task '%16s' [period:%d], deadline:%5d; "
                    "next_period:%5d, WCET:%6d\t\n",
                    timer_get_ticks(), entry->name,
                    /*...*/,
                    /*...*/,
                    /*...*/,
                    /*...*/);
            } else {
                // If the deadline of the process is less than the minimum value, pick it.
                if (/*...*/ < /*...*/) {
                    // Set it as next task.
                    next = entry;
                    // Update the minimum value.
                    min  = /*...*/;
                }
            }
        }
    }
    if (next)
        return next;
#endif
    // If there are no periodic task to execute, just pick an aperiodic task by
    // using Round-Robin.
    return __scheduler_rr(runqueue, false);
}

/// @brief Executes the task with the earliest next PERIOD among all the ready
/// tasks.
/// @details When a task was executed, and its period is starting again, it must
/// be set as 'executable again', and its deadline and next_period must be
/// updated.
/// @param runqueue list of all processes.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_rm(runqueue_t *runqueue)
{
#ifdef SCHEDULER_RM
    // This will hold the pointer to the next task to schedule.
    task_struct *next  = NULL;
    // This will hold a given entry, while iterating the list of tasks.
    task_struct *entry = NULL;
    // Initialize the nearest "next deadline".
    time_t min         = UINT_MAX;

    // Iter over the runqueue to find the task with the earliest absolute deadline.
    list_for_each_decl (it, &runqueue->queue) {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is not a periodic task, skip it.
        if (!__is_periodic_task(entry))
            continue;

        if (entry->se.executed) {
            // If the period for the entry is starting again and it has already
            //  executed, set it as 'executable again'. Deadline and next_period
            //  are propagated.
            if (/*...*/ <= /*...*/) {
                entry->se.executed = /*...*/;
                entry->se.deadline += /*...*/;
                entry->se.next_period += /*...*/;
                pr_debug(
                    "[%9d] Activating task '%16s' [period:%d], deadline:%5d; "
                    "next_period:%5d, WCET:%6d\t\n",
                    timer_get_ticks(), entry->name,
                    /*...*/,
                    /*...*/,
                    /*...*/,
                    /*...*/);
            } else {
                // If the next period of the process is less than the minimum value, pick it.
                if (/*...*/ < /*...*/) {
                    // Set it as next task.
                    next = entry;
                    // Update the minimum value.
                    min  = /*...*/;
                }
            }
        }
    }
    if (next)
        return next;
#endif
    // If there are no periodic task to execute, just pick an aperiodic task by
    // using Round-Robin.
    return __scheduler_rr(runqueue, false);
}

task_struct *scheduler_pick_next_task(runqueue_t *runqueue)
{
    // Update task statistics.
    __update_task_statistics(runqueue->curr);

    // Pointer to the next task to schedule.
    task_struct *next = NULL;
#if defined(SCHEDULER_RR)
    next = __scheduler_rr(runqueue, false);
#elif defined(SCHEDULER_PRIORITY)
    next = __scheduler_priority(runqueue, false);
#elif defined(SCHEDULER_CFS)
    next = __scheduler_cfs(runqueue, false);
#elif defined(SCHEDULER_EDF)
    next = __scheduler_edf(runqueue);
#elif defined(SCHEDULER_RM)
    next = __scheduler_rm(runqueue);
#elif defined(SCHEDULER_AEDF)
    next = __scheduler_aedf(runqueue);
#else
#error "You should enable a scheduling algorithm!"
#endif

    assert(next && "No valid task selected by the scheduling algorithm.");

    // Update the last context switch time of the next task.
    next->se.exec_start = timer_get_ticks();

#ifdef ENABLE_SCHEDULER_FEEDBACK
    // Update the feedback statistics for the new scheduled task.
    scheduler_feedback_task_update(next);
    // Update the overall feedback system.
    scheduler_feedback_update();
#endif
    return next;
}

static void __update_task_statistics(task_struct *task)
{
    // See `prio.h` for more support functions.
#if defined(SCHEDULER_CFS) || defined(SCHEDULER_EDF) || defined(SCHEDULER_RM) || defined(SCHEDULER_AEDF)
    assert(task && "Current task is not valid.");

    // While periodic task is under analysis is executed with aperiodic
    // scheduler and can be preempted by a "true" periodic task.
    // We need to sum all the execution spots to calculate the WCET even
    // if is a more pessimistic evaluation.
    // Update the delta exec.
    task->se.exec_runtime = timer_get_ticks() - task->se.exec_start;

    // Perform timer-related checks.
    update_process_profiling_timer(task);

    // Set the sum_exec_runtime.
    task->se.sum_exec_runtime += task->se.exec_runtime;

    // If the task is not a periodic task we have to update the virtual runtime.
    if (!task->se.is_periodic) {
        // Get the weight of the current task.
        time_t weight = GET_WEIGHT((task)->se.prio); /* ... */
        ;
        // If the weight is different from the default load, compute it.
        if (weight != NICE_0_LOAD) {
            // Get the multiplicative factor for its delta_exec.
            double factor = ((double)NICE_0_LOAD / (double)weight); /* ... */
            ;
            // Weight the delta_exec with the multiplicative factor.
            task->se.exec_runtime = ((int)(((double)task->se.exec_runtime) * factor)); /* ... */
        }
        // Update vruntime of the current task.
        task->se.vruntime += task->se.exec_runtime;
    }
#endif
}
