#include "atomic_broadcast_ring.h"
#include <iostream>

AtomicBroadcastRing::AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes)
    : ReliableComm(id, nodes, "AB") {
    // Determine the next node in the ring
    auto it = nodes.find(id);
    if (++it == nodes.end()) {
        it = nodes.begin();
    }
    next_node_id = it->first;
}

int AtomicBroadcastRing::broadcast(const std::vector<uint8_t>& message) {
    log("AB: Broadcasting message to node " + std::to_string(next_node_id));
    //std::cout << "Atomic Broadcast Ring: Broadcasting message to node " << next_node_id << std::endl;
    send(next_node_id, message);
    return 0;
}

Message AtomicBroadcastRing::deliver() {
    while (true) {
        Message msg = receive();

        // Deliver message to application
        log("AB: Delivering message from node "  + std::to_string(msg.sender_id ));

        //std::cout << "Atomic Broadcast Ring: Delivering message from node " << msg.sender_id << std::endl;

        // Forward the message to the next node in the ring unless it's the sender
        if (msg.sender_id != process_id) {
            send(next_node_id, msg.content);
        }
        return msg;
    }
}
