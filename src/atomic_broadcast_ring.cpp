#include "atomic_broadcast_ring.h"
#include <iostream>

class AtomicBroadcastRing {
public:
    AtomicBroadcastRing(int process_id, ReliableComm* reliable_comm, const std::map<int, int>& ring);
    void broadcast(const Message& message);
    Message deliver();

private:
    int process_id;
    int next_process; // Next process in the ring
    ReliableComm* reliable_comm;
    int sequence_number = 0; // Local sequence number for ordering

    std::mutex mtx;
    std::condition_variable cv;

    std::set<int> delivered_messages; // Track delivered message IDs
    std::map<int, Message> pending_messages; // Holds pending messages for delivery

    void forward_message(const Message& message);
    void process_received_message(const Message& message);
};

// Constructor
AtomicBroadcastRing::AtomicBroadcastRing(int process_id, ReliableComm* reliable_comm, const std::map<int, int>& ring)
    : process_id(process_id), reliable_comm(reliable_comm) {
    next_process = ring.at(process_id); // Find the next process in the ring
}

// Broadcast message by circulating it through the ring
void AtomicBroadcastRing::broadcast(const Message& message) {
    // Set sequence number for this message
    Message ordered_message = message;
    ordered_message.sequence_number = ++sequence_number; // Assign sequence number

    // Forward message to next process in the ring
    forward_message(ordered_message);
}

// Forward message to the next process in the ring
void AtomicBroadcastRing::forward_message(const Message& message) {
    reliable_comm->send_message(next_process, message); // Send the message using reliable broadcast
}

void AtomicBroadcastRing::process_received_message(const Message& message) {
    std::unique_lock<std::mutex> lock(mtx);

    // Check if this message has already been delivered
    if (delivered_messages.find(message.sequence_number) != delivered_messages.end()) {
        return;
    }

    // Add message to pending messages and mark it as delivered
    delivered_messages.insert(message.sequence_number);
    pending_messages[message.sequence_number] = message;

    if (message.sender_id != process_id) {
        forward_message(message);
    }

    // Notify that a new message is ready for delivery
    cv.notify_one();
}

// Deliver messages in agreed order
Message AtomicBroadcastRing::deliver() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !pending_messages.empty(); });

    // Retrieve the next message in order
    auto it = pending_messages.begin();
    Message next_message = it->second;
    pending_messages.erase(it); // Remove from pending

    return next_message;
}
