#include "performanceMonitor.h"
#include <iostream>

PerformanceMonitor::PerformanceMonitor() : running(false), operationsPerformed(0) {}

void PerformanceMonitor::incrementOperations() {
    operationsPerformed++;
}

void PerformanceMonitor::monitorPerformance() {
    while (running) {
        //int ops = operationsPerformed.exchange(0);
        //std::cout << "Throughput: " << ops << " ops/sec" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void PerformanceMonitor::startMonitoring() {
    running = true;
    std::thread(&PerformanceMonitor::monitorPerformance, this).detach();
}

void PerformanceMonitor::stopMonitoring() {
    running = false;
}
