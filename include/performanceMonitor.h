#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <atomic>
#include <thread>
#include <chrono>

class PerformanceMonitor {
private:
    std::atomic<bool> running;
    std::atomic<int> operationsPerformed;

    void monitorPerformance();

public:
    PerformanceMonitor();
    void incrementOperations();
    void startMonitoring();
    void stopMonitoring();
};

#endif // PERFORMANCEMONITOR_H
