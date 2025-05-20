# WorkStealing

A C++ implementation of work-stealing scheduling for parallel DAG execution.

## Clone
```bash
git clone https://github.com/igholami/WorkStealing.git
cd WorkStealing
```
## Build

```bash
mkdir build && cd build
cmake ..
make
```
## Run

```bash
./ABPScheduler
```

## Description

Each job is a DAG with a set of tasks. The tasks are represented as nodes in the graph, and the edges represent dependencies between tasks. The scheduler uses work-stealing to balance the load across multiple threads.
Class `Task` represents a task in the DAG. It has a unique name, a list of dependencies, and a number of dependent tasks.

Class `Scheduler` is responsible for scheduling tasks. It uses a work-stealing algorithm to balance the load across multiple threads. The scheduler maintains a list of available tasks and a list of threads that are currently executing tasks. When a thread finishes executing a task, it checks if there are any available tasks in the queue. If there are, it steals a task from another thread's queue.

Scheduler uses a `WorkStealingQueue` to manage the tasks. The WorkStealingQueue is a thread-safe queue that allows threads to steal tasks from each other. The queue uses a lock-free algorithm to ensure that multiple threads can access it simultaneously without blocking each other.
The WorkStealingQueue is implemented using in a fxied size array.

Each thread has its own logger, which is used to log the tasks that are executed by that thread. The logger is implemented using a thread-safe queue, which allows multiple threads to log messages simultaneously without blocking each other.

Lastly, by gathering the logs from all threads, we can create a complete log of the tasks that were executed by the scheduler. This log can be used to analyze the performance of the scheduler and identify any bottlenecks in the execution of tasks.
