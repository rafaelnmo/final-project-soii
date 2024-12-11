#ifndef GENERATOR_H
#define GENERATOR_H

#include <vector>
#include <thread>
#include "atomic_broadcast_ring.h"
#include "request.h"

class generator
{
private:
    /* data */
    std::vector<std::thread> generator_threads;
    std::queue<Request> message_queue;
    std::mutex queue_mtx;
    int write_chance;
    int total_ops;
    ReliableComm* comm;
    std::condition_variable cv_send;
    std::mutex mtx;
    int bd_id;
public:
    generator(int num_threads, int write_chance, int total_ops, ReliableComm* comm);
    ~generator();
    void generate_message();
    void send_message();
    void run();
};

#endif