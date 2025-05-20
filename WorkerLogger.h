//
// Created by Iman Gholami on 5/20/25.
//

#ifndef ABPSCHEDULER_WORKERLOGGER_H
#define ABPSCHEDULER_WORKERLOGGER_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <ctime>
#include <string>


class WorkerLogger {
private:
    std::ofstream log_file;
    size_t worker_id;
public:
    explicit WorkerLogger(const size_t worker_id) {
        std::cerr << "Creating logger for worker " << worker_id << "\n";
        log_file.open("worker_" + std::to_string(worker_id) + ".log", std::ios::out);
        if (!log_file.is_open()) {
            std::cerr << "Failed to open log file for worker " << worker_id << "\n";
        }
        this->worker_id = worker_id;
    }

    void log(const std::string& msg) {
        log(msg.c_str());
    }

    void log(const char* msg) {
        if (!log_file.is_open()) return;

        using namespace std::chrono;
        auto now = system_clock::now();
        std::time_t now_time_t = system_clock::to_time_t(now);
        auto us = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;  // microseconds modulo 1,000,000
        std::tm* now_tm = std::localtime(&now_time_t);

        log_file << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
                 << '.' << std::setfill('0') << std::setw(6) << us.count()
                 << " " << msg << "\n";
        log_file.flush();
        std::cerr << worker_id << ":" << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
                  << '.' << std::setfill('0') << std::setw(6) << us.count()
                  << " " << msg << "\n";
        std::cerr.flush();
    }

    ~WorkerLogger() {
        std::cerr << "Destroying logger for worker " << worker_id << "\n";
        if (log_file.is_open()) {
            log_file.close();
        }
    }
};


#endif //ABPSCHEDULER_WORKERLOGGER_H
