#include "bd.h"
#include "request.h"
#include <iostream>
#include <fstream>
#include <unistd.h>

bd::bd(int id, int gen_id, ReliableComm* comm)
{
    this->comm = comm;
    req_amount = 0;
    this->gen_id = gen_id;
    run();
}

bd::~bd()
{
}

void bd::run()
{
    while (true) {
        Message received = comm->receive(); // Receive message from generator

        if (received.msg_type=="ERR") {
            //std::cout << "\n\n ERROR: Nothing to receive \n";
            continue;
        }

        req_amount++;

        Request req = Request::deserialize(received.content);
        if (req.type=="SET") {
            data[req.key] = req.val;
            Request result(req.key, req.val, "YES");
            comm->send(gen_id, result.serialize());
        } else {
            if (data.find(req.key) == data.end()) {
                Request result(-1, -1, "NOO");
                comm->send(gen_id, result.serialize());
            } else {
                std::vector<uint8_t> data_vector(sizeof(int));
                std::memcpy(data_vector.data(), &data[req.key], sizeof(int));
                comm->send(gen_id, data_vector);
            }
        }
    }
}

void bd::throughput()
{
    sleep(1);
    std::ofstream MyFile("filename.txt");
    MyFile << req_amount;
    req_amount = 0;
    MyFile.close();
}
