#ifndef ATOMIC_BROADCAST_RING_H
#define ATOMIC_BROADCAST_RING_H

#include <map>
#include <set>
#include <mutex>
#include <condition_variable>
#include "reliable_comm.h"
#include "message.h" 

class AtomicBroadcastRing {
public:
    AtomicBroadcastRing(int process_id, ReliableComm* reliable_comm, const std::map<int, int>& ring);
    void broadcast(const Message& message);   
    Message deliver();       // Deliver the next message in order

private:
    int process_id; 
    int next_process; // ID of the next process in the ring
    ReliableComm* reliable_comm; 
    int sequence_number; // Local sequence number for ordering

    std::mutex mtx;
    std::condition_variable cv; // Condition variable for waiting on message delivery

    std::set<int> delivered_messages; // Track delivered message IDs
    std::map<int, Message> pending_messages; // map of pending messages for delivery

    // Forward message to the next process in the ring
    void forward_message(const Message& message);
    void process_received_message(const Message& message);
};

#endif
