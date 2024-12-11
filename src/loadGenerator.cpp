#include "loadGenerator.h"
#include <iostream>
#include <chrono>
#include <random>

LoadGenerator::LoadGenerator(int ratio, int operations, int clients, KeyValueStore& kv)
    : readWriteRatio(ratio), totalOperations(operations), numClients(clients), kvStore(kv), operationsPerformed(0) {}

void LoadGenerator::generateRequests() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);

    while (operationsPerformed < totalOperations) {
        int opType = dist(gen);
        std::string key = "key" + std::to_string(operationsPerformed);
        std::string value = "value" + std::to_string(operationsPerformed);

        if (opType <= readWriteRatio) {
            kvStore.write(key, value);
        } else {
            kvStore.read(key);
        }

        operationsPerformed++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate load
    }
}

void LoadGenerator::generateLoad() {
    std::vector<std::thread> threads;

    for (int i = 0; i < numClients; ++i) {
        threads.emplace_back(&LoadGenerator::generateRequests, this);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
