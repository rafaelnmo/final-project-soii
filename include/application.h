#ifndef APPLICATION_H
#define APPLICATION_H

#include "reliable_comm.h"
#include "atomic_broadcast_ring.h"
#include <iostream>

class Application {
public:
    Application(ReliableComm* comm, AtomicBroadcastRing* atomic_broadcast);
    void run(int process_id, const std::string& broadcast_type);

private:
    ReliableComm* comm;
    AtomicBroadcastRing* atomic_broadcast;

    void reliable_broadcast_run(int process_id);
    void atomic_broadcast_run(int process_id);
};

#endif