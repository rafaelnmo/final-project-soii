#ifndef BD_H
#define BD_H

#include "reliable_comm.h"
#include "atomic_broadcast_ring.h"

class bd
{
private:
    std::map<int, int> data;    // data of bd
    ReliableComm* comm;
    int req_amount;
    int gen_id;
public:
    bd(int id, int gen_id, ReliableComm* comm);
    ~bd();
    void run();
    void throughput();
};

#endif