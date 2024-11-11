#ifndef ATOMIC_BROADCAST_RING_H
#define ATOMIC_BROADCAST_RING_H

#include "reliable_comm.h"
#include "message.h"
#include "channels.h"
#include <vector>
#include <map>
#include <cstdint>

class AtomicBroadcastRing : public ReliableComm {
public:

    AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes);
    int broadcast(const std::vector<uint8_t>& message) override;
    int broadcast_ring(const std::vector<uint8_t>& message);

    Message deliver() override;
    void listen();
    int find_next_node(int id);

private:
    int next_node_id;
    bool token = false;
};

#endif 
