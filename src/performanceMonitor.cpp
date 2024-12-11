#include "performanceMonitor.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

PerformanceMonitor::PerformanceMonitor(Logger* logger)
    : running(false), operationsPerformed(0), lastTimestamp(std::chrono::steady_clock::now()), logger(logger) {}

void PerformanceMonitor::incrementOperations() {
    operationsPerformed++;
}

void PerformanceMonitor::monitorPerformance() {
    while (running) {
        // Wait for 1 second to calculate throughput
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto currentTimestamp = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTimestamp - lastTimestamp;

        // Calculate throughput in operations per second
        int ops = operationsPerformed.load();  // Get the current value without resetting
        double throughput = ops / elapsed.count();  // Operations per second

        // Log elapsed time and throughput to the console and the log file
        std::cout << "\nElapsed time: " << elapsed.count() << " seconds" << std::endl;
        std::cout << "Throughput: " << throughput << " ops/sec" << std::endl;

        // Log to file using Logger
        if (logger != nullptr) {  // Ensure logger is not null
            logger->log("Elapsed time: " + std::to_string(elapsed.count()) + " seconds");
            logger->log("Throughput: " + std::to_string(throughput) + " ops/sec");
        }

        // Reset the counter after calculating throughput
        operationsPerformed.store(0);

        // Update the last timestamp
        lastTimestamp = currentTimestamp;
    }
}

void PerformanceMonitor::startMonitoring() {
    running = true;
    std::thread(&PerformanceMonitor::monitorPerformance, this).detach();
}

void PerformanceMonitor::stopMonitoring() {
    running = false;
}