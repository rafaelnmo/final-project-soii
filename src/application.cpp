#include "application.h"
#include <iostream>

Application::Application(ReliableComm* comm)
    : comm(comm) {}

void Application::run() {
    // Example usage
    std::vector<uint8_t> message = { 'H', 'e', 'l', 'l', 'o' };

    // Send a message to process 1
    comm->send(1, message);

    // Broadcast a message to all processes
    comm->broadcast(message);

    // Wait to receive a message
    Message received = comm->receive();
    std::cout << "Received message from process " << received.sender_id << ": ";
    for (auto byte : received.content) {
        std::cout << byte;
    }
    std::cout << std::endl;
}