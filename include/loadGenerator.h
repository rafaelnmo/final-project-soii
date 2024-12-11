#ifndef LOADGENERATOR_H
#define LOADGENERATOR_H

#include "keyValueStore.h"
#include <thread>
#include <vector>
#include <atomic>

class LoadGenerator {
private:
    int readWriteRatio;
    int totalOperations;
    int numClients;
    KeyValueStore& kvStore;
    std::atomic<int> operationsPerformed;

    void generateRequests();

public:
    LoadGenerator(int ratio, int operations, int clients, KeyValueStore& kv);
    void generateLoad();
};

#endif // LOADGENERATOR_H
