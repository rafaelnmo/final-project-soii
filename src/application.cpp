#include "application.h"
#include <iostream>

Application::Application(ReliableComm* comm)
    : comm(comm) {}

void Application::run(int process_id) {

    std::vector<uint8_t> message1 = { 'F', 'i', 'r', 's', 't' };
    std::vector<uint8_t> message2 = { 'S', 'e', 'c', 'o', 'n', 'd' };
    std::vector<uint8_t> message3 = { 'T', 'h', 'i', 'r', 'd' };

    if (process_id==0) {

        std::cout << "Broadcasting message1 -----" << std::endl;
        int status = comm->broadcast(message1);
        std::cout << "Status do envio: " << status << std::endl;

        std::cout << "Broadcasting message2 -----" << std::endl;
        status = comm->broadcast(message2);
        std::cout << "Status do envio: " << status << std::endl;

        // std::cout << "Broadcasting message3 -----" << std::endl;
        // status = comm->broadcast(message3);
        // std::cout << "Status do envio: " << status << std::endl;

    } else {
        for (int i=0; i<2; i++) {
            // Wait to receive a message
            Message received = comm->deliver();
            std::cout << "\n\nReceived message from process " << received.sender_id << ": \n";
            for (auto byte : received.content) {
                std::cout << byte << std::endl;
            }
            std::cout << std::endl;
        }
    }
}
