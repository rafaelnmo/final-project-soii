#include "application.h"
#include <iostream>

Application::Application(ReliableComm* comm)
    : comm(comm) {}

void Application::run() {
    // Example usage
    std::vector<uint8_t> content = { 'H', 'e', 'l', 'l', 'o' };
    Message original_msg(0, content);
    std::vector<uint8_t> serialized_msg = original_msg.serialize();

    std::cout << "Serialized data: ";
    for (uint8_t byte : serialized_msg) {
        std::cout << static_cast<int>(byte) << " ";
    }
    std::cout << std::endl;

    Message deserialized_msg = Message::deserialize(serialized_msg);
    std::cout << "Deserialized data: " << std::endl;
    std::cout << "ID: " << deserialized_msg.sender_id << std::endl;
    std::cout << "Content: ";
    for (uint8_t byte : deserialized_msg.content) {
        std::cout << byte;
    }
    std::cout << std::endl;

    // Send a message to process 1
    comm->send(1, serialized_msg);

    // Wait to receive a message
    Message received = comm->receive();
    std::cout << "Received message from process " << received.sender_id << ": ";
    for (auto byte : received.content) {
        std::cout << byte;
    }
    std::cout << std::endl;
}