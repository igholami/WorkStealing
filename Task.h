//
// Created by Iman Gholami on 5/20/25.
//

#ifndef ABPSCHEDULER_TASK_H
#define ABPSCHEDULER_TASK_H

#include <atomic>
#include <vector>
#include <string>
#include <functional>

/**
 * Task node representing a vertex in the input DAG.
 * `remaining_predecessors` counts unfinished incoming edges.
 * When it reaches zero the task becomes ready and can be executed.
 * `successors` is a list of outgoing edges.
 * `fn` is the function to execute.
 * `name` is a human-readable name for the task.
 */
class Task {
public:
    std::function<void()> fn;
    std::atomic<int> remaining_predecessors{0};
    std::vector<Task *> successors;
    std::string name;

    Task() = default;

    Task(std::string name, std::function<void()> f) : name(name), fn(std::move(f)) {}
};


#endif //ABPSCHEDULER_TASK_H
