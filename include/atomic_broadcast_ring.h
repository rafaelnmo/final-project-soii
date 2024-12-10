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
#include <set>
#include <atomic>

// Participant States
enum class ParticipantState {
    Uninitialized,  // 0
    Active,         // 1
    Suspicious,     // 2
    Defective       // 3
};

class AtomicBroadcastRing : public ReliableComm {
public:

    // Constructor with extended group functionality
    AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes,
        const std::string& conf, int chance, int delay,
        const std::map<std::string, std::set<int>>& initial_groups = {});

    int broadcast(const std::vector<uint8_t>& message, const std::string& group_name);
    
    // ----- GROUPS -----
    // Broadcast a message to a specific group
    int broadcast_ring(const std::vector<uint8_t>& message, int max_attempts, const std::string& group_name);
    // Method to dynamically create a new group
    void create_group(const std::string& group_name);
    // Method to join an existing group
    int join_group(const std::string& group_name);
    std::mutex mtx_join; // Mutex for synchronizing access to join_queue
    std::queue<Message> join_queue;
    std::condition_variable cv_join;             // Condition variable for signaling join_queue


    Message deliver() override;
    void listen();
    
    // Participant states (map of process ID to state)
    std::map<int, ParticipantState> participant_states;
    void detect_defective_processes();  // Detect faulty processes based on heartbeats
    int send_token();
    void buffer_message_for_uninitialized(const Message& msg);  // Buffer messages for uninitialized processes
    void deliver_buffered_messages(int node_id);  // Deliver buffered messages once the process is active
    void send_heartbeat();  // Periodically send heartbeat messages
    std::map<int, std::queue<Message>> message_buffers;
    std::queue<Message> deliver_queue;
    int leave_group(const std::string& group_name);
    void handle_leave_message(const Message& msg);


private:
    // Witness of nodes (map of process ID to set of process IDs that mark it as alive)
    std::map<int, std::set<uint8_t>> witness_of_nodes;

    // Participant states (map of process ID to state)
    //std::map<int, ParticipantState> participant_states;

    // Buffers to store messages for uninitialized processes
    //std::map<int, std::queue<Message>> message_buffers;

    // ----- GROUPS -----
    // Group-related data structures
    std::map<std::string, std::set<int>> groups; // Groups and their members
    std::set<std::string> active_groups;         // Groups this node is active in
    std::mutex group_mtx;                        // Mutex for synchronizing group data

    // Serialize and deserialize group join messages
    std::vector<uint8_t> serialize_group_message(const std::string& group_name, int node_id);
    std::pair<std::string, int> deserialize_group_message(const std::vector<uint8_t>& msg);
    // Process incoming join messages
    void process_join_message(const Message& msg);

    // ----------------

    int next_node_id;
    bool token = false;
    // std::queue<Message> deliver_queue;
    std::condition_variable cv_deliver;
    std::mutex mtx_deliver;

    std::condition_variable cv_token;
    std::condition_variable cv_token_monitor;
    std::mutex mtx_token;
    std::mutex mtx_token_monitor;

    bool tkt_passsed = false;

    std::map<int, std::queue<int>> group_members;
    std::map<int, bool> has_group_token;

    // Heartbeat interval (in milliseconds)
    int heartbeat_interval = 1000;  // Default is 1 second
    int failures;

    std::condition_variable cv_send_htb;
    std::mutex mtx_send_htb;
    bool send_htb = false;

    // Failure detection methods
    int find_next_node(int key);    // check for next active node in ring
    // void send_heartbeat();  // Periodically send heartbeat messages
    void process_heartbeat(const Message& msg);  // Process received heartbeat messages
    //void detect_defective_processes();  // Detect faulty processes based on heartbeats
    //void buffer_message_for_uninitialized(const Message& msg);  // Buffer messages for uninitialized processes
    //void deliver_buffered_messages(int node_id);  // Deliver buffered messages once the process is active


    static void signalHandler(int signum);
    //int send_token();

    void token_monitor();
    void deliver_thread();

    int find_key(std::string address);
    void print_states();
    void htb_handler_thread();
    void receive_join();
};

#endif
