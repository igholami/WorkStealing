#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <random>
#include <optional>
#include <array>

#include "WorkerLogger.h"
#include "Scheduler.h"


void simulate_work(int iterations) {
    for (volatile int i = 0; i < iterations; ++i) {}
}

Task* generate_random_dag_with_source_and_sink(int num_tasks, double avg_seconds_per_task) {
    std::vector<Task*> heap_tasks;
    heap_tasks.reserve(num_tasks);

    int base_iterations = static_cast<int>(avg_seconds_per_task * 1'000'000'000);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dep_count_dist(0, 3);
    std::normal_distribution<> work_dist(base_iterations, base_iterations / 2);

    // Create SOURCE task
    Task* source = new Task("SOURCE", [] { simulate_work(5'000'000); });
    heap_tasks.push_back(source);

    // Create other tasks
    for (int i = 1; i < num_tasks; ++i) {
        int iterations = std::max(1000, static_cast<int>(work_dist(gen)));
        std::string name = (i == num_tasks - 1) ? "SINK" : ("TASK_" + std::to_string(i));
        heap_tasks.push_back(new Task(name, [iterations] { simulate_work(iterations); }));
    }

    // SOURCE has no predecessors; connect SOURCE to all other tasks initially
    for (int i = 1; i < num_tasks; ++i) {
        source->successors.push_back(heap_tasks[i]);
        heap_tasks[i]->remaining_predecessors.store(1);
    }

    // Add random extra dependencies among non-source tasks, excluding SINK (no successors)
    for (int i = 2; i < num_tasks - 1; ++i) { // exclude sink from having successors
        int dep_count = dep_count_dist(gen);
        for (int d = 0; d < dep_count; ++d) {
            std::uniform_int_distribution<> dep_task_dist(1, i - 1);
            int dep_idx = dep_task_dist(gen);
            heap_tasks[dep_idx]->successors.push_back(heap_tasks[i]);
            heap_tasks[i]->remaining_predecessors++;
        }
    }

    // Final vertex index (sink) is last task
    int final_task_index = num_tasks - 1;

    // Ensure SINK has no successors (sink)
    heap_tasks[final_task_index]->successors.clear();

    // Ensure final task is the only sinking task
    for (Task* task : heap_tasks) {
        if (task != heap_tasks[final_task_index] && task->successors.empty()) {
            task->successors.push_back(heap_tasks[final_task_index]);
            heap_tasks[final_task_index]->remaining_predecessors++;
        }
    }

    return source;
}

// Deep copy DAG
Task* deep_copy_dag(Task* root, int dag_index) {
    std::unordered_map<Task*, Task*> copied;

    std::function<Task*(Task*)> copy_helper = [&](Task* node) -> Task* {
        if (!node) return nullptr;
        if (copied.count(node)) return copied[node];

        std::string new_name = "DAG_" + std::to_string(dag_index) + "_" + node->name;

        Task* copy = new Task(new_name, node->fn);
        copy->remaining_predecessors.store(node->remaining_predecessors.load());
        copied[node] = copy;

        for (Task* succ : node->successors) {
            copy->successors.push_back(copy_helper(succ));
        }
        return copy;
    };

    return copy_helper(root);
}


int main() {
    WorkerLogger logger(1234);
    Scheduler sched;

    int num_tasks = 100;
    double avg_seconds_per_task = 0.1;
    Task* root = generate_random_dag_with_source_and_sink(num_tasks, avg_seconds_per_task);

    int num_dags = 20;
    std::vector<Task*> dags;
    for (int i = 0; i < num_dags; ++i) {
        Task* copied_dag = deep_copy_dag(root, i);
        dags.push_back(copied_dag);
    }

    for (int i = 0; i < num_dags; ++i) {
        Task* dag = dags[i];
        logger.log("Worker 1234 submitting task " + dag->name);
        sched.submit(dag);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate lag
    }

    std::this_thread::sleep_for(std::chrono::seconds(200));
    return 0;
}