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
    //Application(ReliableComm* comm);
    Application(AtomicBroadcastRing* comm);
    void run(int process_id);

private:
    //ReliableComm* comm;
    AtomicBroadcastRing* comm;
    KeyValueStore* kvStore;
    Logger* logger;
    PerformanceMonitor* monitor;
    LoadGenerator* loadGen;

    void initialize(int process_id);


};

#endif