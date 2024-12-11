#ifndef APPLICATION_H
#define APPLICATION_H

#include "reliable_comm.h"
#include "atomic_broadcast_ring.h"
#include "keyValueStore.h"
#include "loadGenerator.h"
#include "performanceMonitor.h"
#include "logger.h"
#include <map>
#include <set>
#include <string>

class Application {
public:
    Application(AtomicBroadcastRing* comm);
    void run(int process_id);

private:
    AtomicBroadcastRing* comm;
    std::map<int, std::pair<std::string, int>> nodes;
    std::map<std::string, std::set<int>> groups;
    AtomicBroadcastRing* atomicRing;
    KeyValueStore* kvStore;
    Logger* logger;
    PerformanceMonitor* monitor;
    LoadGenerator* loadGen;

    void initialize(int process_id);
};

#endif // APPLICATION_H

// #ifndef APPLICATION_H
// #define APPLICATION_H

// #include "reliable_comm.h"

// class Application {
// public:
//     Application(ReliableComm* comm);
//     void run(int process_id);

// private:
//     ReliableComm* comm;
// };

// #endif
