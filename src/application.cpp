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
        std::cout << "Broadcasting message1 -----" << std::endl;
        int status = comm->broadcast(message1);
        std::cout << "Resultado do envio: " << status << std::endl;
        // std::cout << "Broadcasting message2 -----" << std::endl;
        // comm->broadcast(message2);
    } else {
        for (int i=0; i<1; i++) {
            // Wait to receive a message
            Message received = comm->deliver();
            std::cout << "\n\nReceived message from process " << received.sender_id << ": ";
            for (auto byte : received.content) {
                std::cout << byte << std::endl;
            }
            std::cout << std::endl;
        }
    }
}