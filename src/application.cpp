#include "application.h"
#include <iostream>

Application::Application(ReliableComm* comm)
    : comm(comm) {}

void Application::run(int process_id) {
    if (process_id==0) {
        // Example usage
        std::vector<uint8_t> message1 = { 'H', 'e', 'l', 'l', 'o' };
        std::vector<uint8_t> message2 = { 'G','o','o','d','b','y','e'};

        // Send a message to process 1
        comm->send(1, message1);
        comm->send(1, message2);

        // Broadcast a message to all processes
        //comm->broadcast(message);
    
    } else {
        while (true) {
            // Wait to receive a message
            Message received = comm->receive();
            std::cout << "Received message from process " << received.sender_id << ": ";
            for (auto byte : received.content) {
                std::cout << byte << std::endl;
            }
        }
    }
}