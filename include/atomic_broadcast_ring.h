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
    int broadcast_ring(const std::vector<uint8_t>& message, int max_attempts);

    Message deliver() override;
    void listen();
    //int find_next_node(int id);

private:
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

    static void signalHandler(int signum);
    int send_token();

    void token_monitor();
    void deliver_thread();
};

#endif
