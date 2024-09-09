#ifndef RELIABLE_COMM_H
#define RELIABLE_COMM_H

#include "message.h"
#include <map>
#include <set>
#include <queue>
#include <mutex>
#include <condition_variable>

class Channels;
class FailureDetection;

class ReliableComm {
public:
    ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes);
    void send(int id, const std::vector<uint8_t>& message);
    void broadcast(const std::vector<uint8_t>& message);
    Message receive();
    Message deliver();

private:
    int process_id;
    std::map<int, std::pair<std::string, int>> nodes;
    std::set<int> delivered_messages;
    
    Channels* channels;
    FailureDetection* failure_detection;
    
    std::mutex mtx;
    std::condition_variable cv;
    std::queue<Message> message_queue;

    void listen();
    void send_message(int id, const std::vector<uint8_t>& message);
    bool is_delivered(int msg_hash);
    void mark_delivered(int msg_hash);
};

#endif
