//
// Created by Iman Gholami on 5/20/25.
//

#ifndef ABPSCHEDULER_SCHEDULER_H
#define ABPSCHEDULER_SCHEDULER_H

#include <vector>
#include <thread>
#include <random>
#include <optional>
#include <atomic>
#include <iostream>

#include "WorkerLogger.h"
#include "Task.h"
#include "WorkStealingDeque.h"

/**
 * ABP‑style scheduler for multiple input DAGs.
 * Each worker owns a deque and tries to execute tasks from it.
 * If it fails, it tries to steal from other workers.
 * logger is used to log the actions of each worker.
 */
class Scheduler {
public:
    explicit Scheduler(size_t num_workers = std::thread::hardware_concurrency())
            :shutdown_(false), num_workers(num_workers) {
        for (size_t i = 0; i < num_workers; ++i) {
            workers_.push_back(std::make_shared<WorkerLocal>(i));
        }
        for (size_t i = 0; i < num_workers; ++i) {
            threads_.emplace_back([this, i] { worker_loop(i); });
        }
    }

    ~Scheduler() {
        shutdown_.store(true, std::memory_order_release);
        for (auto &thr: threads_) thr.join();
    }

    void submit(Task *task, size_t id = 0) {
        while (!workers_[id]->deque.push_bottom(task)) {
            workers_[id]->logger->log("Worker " + std::to_string(id) + " deque full, Yielding");
            std::this_thread::yield();
        }
    }

    /**
     * Mark `task` finished and propagate readiness to its successors.
     */
    void execute(Task *task, size_t wid) {
        workers_[wid]->logger->log("Worker " + std::to_string(wid) + " start executing task " + task->name);
        task->fn(); // execute the task
        workers_[wid]->logger->log("Worker " + std::to_string(wid) + " finished executing task " + task->name);
        for (Task *succ: task->successors) {
//            workers_[wid]->logger->log("Worker " + std::to_string(wid) + " decrementing successor " + succ->name);
            if (succ->remaining_predecessors.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                // became ready
                workers_[wid]->logger->log("Worker " + std::to_string(wid) + " makes task " + succ->name + " ready");
                submit(succ, wid);
            }
        }
    }

private:
    struct WorkerLocal {
        WorkStealingDeque<Task> deque;
        std::shared_ptr<WorkerLogger> logger;
        explicit WorkerLocal(size_t wid) : deque() {
            logger = std::make_shared<WorkerLogger>(wid);
        }
    };

    std::vector<std::shared_ptr<WorkerLocal>> workers_;
    size_t num_workers;
    std::vector<std::thread> threads_;
    std::atomic<bool> shutdown_;
    std::mt19937 rng_{std::random_device{}()};

    /**
     * Main loop executed by every worker thread.
     */
    void worker_loop(size_t wid) {
        auto &local = workers_[wid];
        std::mt19937 prng(std::random_device{}());
        const size_t N = workers_.size();

        while (!shutdown_.load(std::memory_order_acquire)) {
            // 1) try own deque
            if (auto opt = local->deque.pop_bottom()) {
                Task *task = *opt;
                workers_[wid]->logger->log("Worker " + std::to_string(wid) + " taking task " + task->name + " from own deque");
                execute(task, wid);
                continue;
            }

            // 2) steal from others
            bool stolen = false;
            for (size_t attempt = 0; attempt < N - 1; ++attempt) {
                size_t victim = (wid + 1 + prng() % (N - 1)) % N;
                if (auto opt = workers_[victim]->deque.steal_top()) {
                    Task *task = *opt;
                    workers_[wid]->logger->log("Worker " + std::to_string(wid) + " stole task " + task->name + " from worker " + std::to_string(victim));
                    execute(task, wid);
                    stolen = true;
                    break;
                }
            }

            if (!stolen) {
                std::this_thread::yield(); // back‑off
            }
        }
    }
};

#endif //ABPSCHEDULER_SCHEDULER_H
