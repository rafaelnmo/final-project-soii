#include "application.h"
#include <iostream>

Application::Application(ReliableComm* comm)
    : comm(comm) {}

void Application::run(int process_id) {
    // Example usage
    std::vector<uint8_t> message1 = { 'H', 'e', 'l', 'l', 'o' };
    std::vector<uint8_t> message2 = { 'G','o','o','d','b','y','e'};
    if (process_id==0) {
        // Send a message to process 1
        comm->send(1, message1);
        comm->send(1, message2);

        for (int i=0; i<2; i++) {
            // Wait to receive a message
            Message received = comm->receive();
            std::cout << "Received message from process " << received.sender_id << ": ";
            for (auto byte : received.content) {
                std::cout << byte << std::endl;
            }
        }
    } else {
        for (int i=0; i<2; i++) {
            // Wait to receive a message
            Message received = comm->receive();
            std::cout << "Received message from process " << received.sender_id << ": ";
            for (auto byte : received.content) {
                std::cout << byte << std::endl;
            }
        }

        comm->send(0, message1);
        comm->send(0, message2);
    }
}