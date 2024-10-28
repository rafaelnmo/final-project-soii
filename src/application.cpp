#include "application.h"
#include <iostream>

// Application::Application(ReliableComm* comm)
//     : comm(comm) {}

// void Application::run(int process_id) {
//     // Example usage
//     std::vector<uint8_t> message1 = { 'H', 'e', 'l', 'l', 'o' };
//     if (process_id==0) {
//         // Send message1
//         std::cout << "Broadcasting message1 -----" << std::endl;
//         int status = comm->broadcast(message1);
//         std::cout << "Status do envio: " << status << std::endl;
//     } else {
//         for (int i=0; i<1; i++) {
//             // Wait to receive a message
//             Message received = comm->deliver();
//             std::cout << "\n\nReceived message from process " << received.sender_id << ": ";
//             for (auto byte : received.content) {
//                 std::cout << byte << std::endl;
//             }
//             std::cout << std::endl;
//         }
//     }
// }


Application::Application(ReliableComm* comm, AtomicBroadcastRing* atomic_broadcast)
    : comm(comm), atomic_broadcast(atomic_broadcast) {}

void Application::run(int process_id, const std::string& broadcast_type) {
    if (broadcast_type == "BE" || broadcast_type == "UR" ) {
        std::cout << "Executando Reliable Broadcast\n";
        reliable_broadcast_run(process_id);
    } else if (broadcast_type == "AB") {
        std::cout << "Executando Atomic Broadcast\n";
        atomic_broadcast_run(process_id);
    } else {
        std::cerr << "Tipo de broadcast desconhecido. Escolha 'reliable' ou 'atomic'.\n";
    }
}

void Application::reliable_broadcast_run(int process_id) {
    std::vector<uint8_t> message1 = { 'H', 'e', 'l', 'l', 'o' };

    if (process_id == 0) {
        std::cout << "Reliable Broadcast: Enviando mensagem1 -----\n";
        int status = comm->broadcast(message1);
        std::cout << "Status do envio: " << status << "\n";
    } else {
        for (int i = 0; i < 1; i++) {
            Message received = comm->deliver();
            std::cout << "\nReliable Broadcast: Mensagem recebida de processo " << received.sender_id << ": ";
            for (auto byte : received.content) {
                std::cout << byte << ' ';
            }
            std::cout << "\n";
        }
    }
}

void Application::atomic_broadcast_run(int process_id) {
    std::vector<uint8_t> message1 = { 'A', 't', 'o', 'm', 'i', 'c' };

    if (process_id == 0) {
        std::cout << "Atomic Broadcast: Enviando mensagem1 -----\n";
        Message atomic_message(process_id, 1, 1, 0, message1);
        atomic_broadcast->broadcast(atomic_message);
    } else {
        for (int i = 0; i < 1; i++) {
            Message received = atomic_broadcast->deliver();
            std::cout << "\nAtomic Broadcast: Mensagem recebida de processo " << received.sender_id << ": ";
            for (auto byte : received.content) {
                std::cout << byte << ' ';
            }
            std::cout << "\n";
        }
    }
}

