#include "reliable_comm.h"
#include "channels.h"
#include "failure_detection.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#define MAX_BUFFER_SIZE 1024

ReliableComm::ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes)
    : process_id(id), nodes(nodes) {
    // Initialize Channels and FailureDetection
    channels = new Channels(nodes);
    failure_detection = new FailureDetection();

    // Create listener thread
    std::thread listener(&ReliableComm::listen, this);
    listener.detach();
}

void ReliableComm::send(int id, const std::vector<uint8_t>& message) {
    send_message(id, message);
}

void ReliableComm::broadcast(const std::vector<uint8_t>& message) {
    for (const auto& node : nodes) {
        if (node.first != process_id) {
            send_message(node.first, message);
        }
    }
}

void ReliableComm::send_message(int id, const std::vector<uint8_t>& message) {
    channels->send_message(id, message);
}

bool ReliableComm::is_delivered(int msg_hash) {
    return delivered_messages.find(msg_hash) != delivered_messages.end();
}

void ReliableComm::mark_delivered(int msg_hash) {
    delivered_messages.insert(msg_hash);
}

void ReliableComm::listen() {
    while (true) {
        auto [msg, msg_hash] = channels->receive_message();
        
        std::unique_lock<std::mutex> lock(mtx);
        if (!is_delivered(msg_hash)) {
            mark_delivered(msg_hash);
            message_queue.push(msg);
            cv.notify_all();
        }
    }
}

Message ReliableComm::receive() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !message_queue.empty(); });

    Message msg = message_queue.front();
    message_queue.pop();
    return msg;
}

Message ReliableComm::deliver() {
    return receive();
}