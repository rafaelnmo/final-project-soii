#ifndef RELIABLE_COMM_H
#define RELIABLE_COMM_H

#include "message.h"
#include <map>
#include <set>
#include <queue>
#include <mutex>
#include <condition_variable>

enum State {
    Waiting,
    SendSYN,
    SendMESSAGE,
    ReceiveACK,
    ReceiveMESSAGE,
    ReceiveCLOSE
};

class Channels;

class ReliableComm {
public:
    ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes, const std::string broadcast_type);
    int send(int id, const std::vector<uint8_t>& message);
    int broadcast(const std::vector<uint8_t>& message);
    Message receive();
    Message deliver();
    int get_process_id();

private:
    enum State communication_state;
    int process_id;
    std::map<int, std::pair<std::string, int>> nodes;
    std::string broadcast_type;
    std::set<int> delivered_messages;
    int msg_num;
    
    Channels* channels;
    
    std::mutex mtx;
    std::condition_variable cv;
    std::queue<Message> message_queue;

    void listen();
    int send_message(int id, const std::vector<uint8_t>& message);
    bool is_delivered(int msg_hash);
    void mark_delivered(int msg_hash);

    Message receive_single_msg();
    Message send_syn_and_wait_ack(int id);
    Message send_contents_and_wait_close(int id, const std::vector<uint8_t>& message);
    Message send_ack_recv_contents(int received_sender_id);

    int beb_broadcast(const std::vector<int> id_list, const std::vector<uint8_t> message);
    int urb_broadcast(const std::vector<int> id_list, const std::vector<uint8_t> message);
};

#endif
