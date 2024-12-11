#include "loadGenerator.h"
#include <iostream>
#include <chrono>
#include <random>
#include <random>

LoadGenerator::LoadGenerator(int ratio, int operations, int clients, KeyValueStore& kv, PerformanceMonitor& monitor )
    : readWriteRatio(ratio), totalOperations(operations), numClients(clients), kvStore(kv), performanceMonitor(monitor), operationsPerformed(0) {}

void LoadGenerator::generateRequests() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);

    std::cout << "\nTotal Operations: " + std::to_string(totalOperations) << std::endl;
    while (operationsPerformed < totalOperations) {

        std::cout << "Operations Performed:" + std::to_string(operationsPerformed) << std::endl;
        int opType = dist(gen);
        std::string key = "key" + std::to_string(operationsPerformed);
        std::string value = "value" + std::to_string(operationsPerformed);

        std::cout << "Operation Type: " + std::to_string(opType) << std::endl;
        std::cout << "Operation Ratio:" + std::to_string(readWriteRatio) << std::endl;
        if (opType <= readWriteRatio) {
            std::cout << "WRITE" << std::endl;
            kvStore.write(key, value);
        } else {
            std::cout << "READ" << std::endl;
            kvStore.read(key);
        }

        operationsPerformed++;
        performanceMonitor.incrementOperations();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate load
    }
}

void LoadGenerator::generateLoad() {
    std::vector<std::thread> threads;

    for (int i = 0; i < numClients; ++i) {
        // threads.emplace_back(&LoadGenerator::generateRequests, this);
        std::cout << "NumClients: " + std::to_string(i) << std::endl;
        threads.emplace_back([this]() { this->generateRequests(); });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
