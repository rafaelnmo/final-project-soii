#include "application.h"
#include <iostream>

Application::Application(ReliableComm* comm)
    : comm(comm) {}

void Application::run(int process_id) {
    // Example usage
    std::vector<uint8_t> message1 = { 'H', 'e', 'l', 'l', 'o' };
    if (process_id==0) {
        // Send message1
        std::cout << "Broadcasting message1 -----" << std::endl;
        int status = comm->broadcast(message1);
        std::cout << "Status do envio: " << status << std::endl;
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