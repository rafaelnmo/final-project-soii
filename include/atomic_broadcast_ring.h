#ifndef ATOMIC_BROADCAST_RING_H
#define ATOMIC_BROADCAST_RING_H

#include "reliable_comm.h"
#include "message.h"
#include "channels.h"
#include <vector>
#include <map>
#include <cstdint>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <iostream>

// Participant States
enum class ParticipantState {
    Uninitialized,
    Active,
    Suspicious,
    Defective
};

class AtomicBroadcastRing : public ReliableComm {
public:

    AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes,
                        std::string conf, int chance, int delay);

    int broadcast(const std::vector<uint8_t>& message) override;
    int broadcast_ring(const std::vector<uint8_t>& message, int max_attempts);

    Message deliver() override;
    void listen();
    //int find_next_node(int id);

private:
    // Participant states (map of process ID to state)
    std::map<int, ParticipantState> participant_states;

    // Buffers to store messages for uninitialized processes
    std::map<int, std::queue<Message>> message_buffers;

    int next_node_id;
    bool token = false;
    std::queue<Message> deliver_queue;
    std::condition_variable cv_deliver;
    std::mutex mtx_deliver;

    std::condition_variable cv_token;
    std::condition_variable cv_token_monitor;
    std::mutex mtx_token;
    std::mutex mtx_token_monitor;

    bool tkt_passsed = false;

    // Heartbeat interval (in milliseconds)
    int heartbeat_interval = 1000;  // Default is 1 second
    int failures;

    // Failure detection methods
    void send_heartbeat();  // Periodically send heartbeat messages
    void process_heartbeat(const Message& msg);  // Process received heartbeat messages
    void detect_defective_processes();  // Detect faulty processes based on heartbeats
    void buffer_message_for_uninitialized(const Message& msg);  // Buffer messages for uninitialized processes
    void deliver_buffered_messages(int node_id);  // Deliver buffered messages once the process is active


    static void signalHandler(int signum);
    int send_token();

    void token_monitor();
    void deliver_thread();

    int find_key(std::string address);
    void print_states();
    void htb_handler_thread();
};

#endif
