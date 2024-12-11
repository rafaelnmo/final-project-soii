#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <atomic>
#include <thread>
#include <chrono>
#include "logger.h"  // Include the Logger class header


class PerformanceMonitor {
private:
    std::atomic<bool> running;
    std::atomic<int> operationsPerformed;
    std::chrono::steady_clock::time_point lastTimestamp; // Last timestamp for time calculations
    Logger* logger;  // Logger object to log the performance metrics


    void monitorPerformance();

public:
    // PerformanceMonitor();
    PerformanceMonitor(Logger* logger);

    void incrementOperations();
    void startMonitoring();
    void stopMonitoring();
};

#endif // PERFORMANCEMONITOR_H
