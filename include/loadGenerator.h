#ifndef LOADGENERATOR_H
#define LOADGENERATOR_H

#include "keyValueStore.h"
#include "performanceMonitor.h"
#include <thread>
#include <vector>
#include <atomic>

class LoadGenerator {
private:
    int readWriteRatio;
    int totalOperations;
    int numClients;
    KeyValueStore& kvStore;
    PerformanceMonitor& performanceMonitor; // Reference to PerformanceMonitor
    std::atomic<int> operationsPerformed;


    void generateRequests();
    void generateRequests(int clientId); 

public:
    LoadGenerator(int ratio, int operations, int clients, KeyValueStore& kv, PerformanceMonitor& monitor);
    void generateLoad();
};

#endif // LOADGENERATOR_H
