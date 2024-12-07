
#include "application.h"
#include "reliable_comm.h"
#include "atomic_broadcast_ring.h"
#include "channels.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <algorithm>

#include <cassert>
#include <thread>
#include <chrono>
#include <iostream>
#include <map>
#include <set>

// void test_heartbeat_handling() {
//     // Set up the nodes and groups
//     std::map<int, std::pair<std::string, int>> nodes = {
//         {0, {"localhost", 8080}},
//         {1, {"localhost", 8081}},
//         {2, {"localhost", 8082}},
//     };
//     std::map<std::string, std::set<int>> groups = {
//         {"group1", {0, 1}},
//         {"group2", {1, 2}},
//     };

//     // Create an AtomicBroadcastRing instance
//     AtomicBroadcastRing abr(0, nodes, "config", 50, 100, groups);

//     // Start a thread that will simulate heartbeat sending
//     std::thread heartbeat_thread([&abr]() {
//         abr.send_heartbeat();
//     });

//     // Sleep for some time to allow heartbeat to be sent and processed
//     std::this_thread::sleep_for(std::chrono::seconds(2));

//     // Test if the node has received any heartbeat (check internal state)
//     // (In real test cases, you'd check shared states such as 'witness_of_nodes')

//     assert(abr.participant_states[0] == ParticipantState::Active);
//     assert(abr.participant_states[1] == ParticipantState::Active);

//     heartbeat_thread.join();
//     std::cout << "Test Heartbeat Handling passed!" << std::endl;
// }

void test_heartbeat_handling() {
    // Set up the nodes and groups
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"localhost", 8080}},
        {1, {"localhost", 8081}},
        {2, {"localhost", 8082}},
    };
    std::map<std::string, std::set<int>> groups = {
        {"group1", {0, 1}},
        {"group2", {1, 2}},
    };

    // Create an AtomicBroadcastRing instance
    AtomicBroadcastRing abr(0, nodes, "config", 50, 100, groups);

    // Start a thread that will simulate heartbeat sending
    std::thread heartbeat_thread([&abr]() {
        abr.send_heartbeat();
    });

    // Sleep for some time to allow heartbeat to be sent and processed
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Print the participant states for debugging
    std::cout << "Participant States after heartbeat:" << std::endl;
    for (const auto& entry : abr.participant_states) {
        std::cout << "Node " << entry.first << " state: " << static_cast<int>(entry.second) << std::endl;
    }

    // Test if the node has received any heartbeat (check internal state)
    assert(abr.participant_states[0] == ParticipantState::Active);
    assert(abr.participant_states[1] == ParticipantState::Active);

    heartbeat_thread.join();
    std::cout << "Test Heartbeat Handling passed!" << std::endl;
}


void test_group_management() {
    // Set up nodes and groups
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"localhost", 8080}},
        {1, {"localhost", 8081}},
        {2, {"localhost", 8082}},
    };
    std::map<std::string, std::set<int>> groups = {
        {"group1", {0, 1}},
    };

    // Create an AtomicBroadcastRing instance
    AtomicBroadcastRing abr(0, nodes, "config", 50, 100, groups);

    // Test group creation
    abr.create_group("group2");
    assert(groups.find("group2") != groups.end());
    std::cout << "Group 'group2' created successfully!" << std::endl;

    // Test node joining a group
    abr.join_group("group2");
    assert(groups["group2"].count(0) == 1);
    std::cout << "Node 0 joined group 'group2' successfully!" << std::endl;

    // Test joining an already existing group
    abr.join_group("group1");
    assert(groups["group1"].count(0) == 1);
    std::cout << "Node 0 already in group 'group1'!" << std::endl;

    std::cout << "Test Group Management passed!" << std::endl;
}

void test_defective_process_detection() {
    // Set up nodes
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"localhost", 8080}},
        {1, {"localhost", 8081}},
        {2, {"localhost", 8082}},
    };
    std::map<std::string, std::set<int>> groups = {
        {"group1", {0, 1}},
        {"group2", {1, 2}},
    };

    // Create an AtomicBroadcastRing instance
    AtomicBroadcastRing abr(0, nodes, "config", 50, 100, groups);

    // Simulate defective process (for testing, let's assume node 2 fails)
    abr.participant_states[2] = ParticipantState::Suspicious;

    // Start the defect detection thread
    std::thread defect_detection_thread([&abr]() {
        abr.detect_defective_processes();
    });

    // Sleep for some time to allow defect detection
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Verify if node 2 is detected as defective
    assert(abr.participant_states[2] == ParticipantState::Uninitialized);
    std::cout << "Defective process detection test passed!" << std::endl;

    defect_detection_thread.join();
}

void test_broadcast_and_token_passing() {
    // Set up nodes and groups
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"localhost", 8080}},
        {1, {"localhost", 8081}},
        {2, {"localhost", 8082}},
    };
    std::map<std::string, std::set<int>> groups = {
        {"group1", {0, 1, 2}},
    };

    // Create an AtomicBroadcastRing instance
    AtomicBroadcastRing abr(0, nodes, "config", 50, 100, groups);

    // Test broadcasting a message
    std::vector<uint8_t> message = {'H', 'e', 'l', 'l', 'o'};
    int broadcast_status = abr.broadcast(message, "group1");

    assert(broadcast_status == 0);
    std::cout << "Broadcast successful!" << std::endl;

    // Test token passing
    int token_status = abr.send_token();
    assert(token_status == 0);
    std::cout << "Token passed successfully!" << std::endl;
}

void test_buffering_and_delivering_messages() {
    // Set up nodes
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"localhost", 8080}},
        {1, {"localhost", 8081}},
        {2, {"localhost", 8082}},
    };
    std::map<std::string, std::set<int>> groups = {
        {"group1", {0, 1}},
        {"group2", {1, 2}},
    };

    // Create an AtomicBroadcastRing instance
    AtomicBroadcastRing abr(0, nodes, "config", 50, 100, groups);

    // Simulate message buffering for uninitialized node 2
    Message msg = Message("localhost", 8081, "MSG", {'T', 'E', 'S', 'T'});
    abr.buffer_message_for_uninitialized(msg);

    // Test buffering
    assert(!abr.message_buffers[2].empty());
    std::cout << "Message buffered successfully!" << std::endl;

    // Simulate node 2 becoming active
    abr.participant_states[2] = ParticipantState::Active;

    // Test message delivery
    abr.deliver_buffered_messages(2);
    assert(abr.deliver_queue.size() == 1);
    std::cout << "Buffered message delivered successfully!" << std::endl;
}



int main() {
    test_heartbeat_handling();
    test_group_management();
    test_defective_process_detection();
    test_broadcast_and_token_passing();
    test_buffering_and_delivering_messages();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
