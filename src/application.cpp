// #include "application.h"
// #include <iostream>

// Application::Application(ReliableComm* comm)
//     : comm(comm) {}

// void Application::run(int process_id) {

//     std::vector<uint8_t> message1 = { 'F', 'i', 'r', 's', 't' };
//     std::vector<uint8_t> message2 = { 'S', 'e', 'c', 'o', 'n', 'd' };
//     std::vector<uint8_t> message3 = { 'T', 'h', 'i', 'r', 'd' };

//     if (process_id==0) {
//         for (int i=0; i<15; i++) {
//             std::cout << "Broadcasting message1 -----" << std::endl;
//             int status = comm->send(1, message1);
//             std::cout << "Status do envio: " << status << std::endl;
//         }

//         // std::cout << "Broadcasting message2 -----" << std::endl;
//         // status = comm->send(1, message2);
//         // std::cout << "Status do envio: " << status << std::endl;

//         // std::cout << "Broadcasting message3 -----" << std::endl;
//         // status = comm->broadcast(message3);
//         // std::cout << "Status do envio: " << status << std::endl;

//     } else {
//         while (true) {
//             // Wait to receive a message
//             Message received = comm->receive();
//             if (received.msg_type=="ERR") {
//                 std::cout << "\n\n ERROR: Nothing to receive \n";
//                 continue;
//             }
//             std::cout << "\n\nReceived message from process " << received.sender_address << ": \n";
//             for (auto byte : received.content) {
//                 std::cout << byte << std::endl;
//             }
//             std::cout << std::endl;
//         }
//     }
// }

#include "application.h"
#include <iostream>
#include <stdexcept>

Application::Application(AtomicBroadcastRing* comm)
    : comm(comm), kvStore(nullptr), logger(nullptr),
      monitor(nullptr), loadGen(nullptr) {}

void Application::initialize(int process_id) {
    // Initialize Atomic Broadcast Ring
    //atomicRing = new AtomicBroadcastRing(process_id, nodes, "config.txt", 0, 100, groups);

    // Initialize KeyValueStore
    kvStore = new KeyValueStore(comm);
    //kvStore = new KeyValueStore(atomicRing);

    // Initialize Logger
    logger = new Logger("performance.log");
    logger->log("\nApplication initialized");

    // Initialize Performance Monitor
    monitor = new PerformanceMonitor();

    // Set up Load Generator parameters
    int readWriteRatio = 50; // 70% writes, 30% reads
    int totalOperations = 10;
    int numClients = 5;

    // Initialize Load Generator
    loadGen = new LoadGenerator(readWriteRatio, totalOperations, numClients, *kvStore);
}

void Application::run(int process_id) {
    initialize(process_id);

    logger->log("Starting performance monitoring");
    monitor->startMonitoring();

    logger->log("Starting load generation");
    std::cout << "Starting load generation..." << std::endl;
    if (process_id==0) {
        loadGen->generateLoad();

        monitor->stopMonitoring();
        logger->log("Load generation completed");
    }  else {
        while (true) {
            // Wait to receive a message
            Message received = comm->receive();
            if (received.msg_type=="ERR") {
                std::cout << "\n\n ERROR: Nothing to receive \n";
                continue;
            }
            std::cout << "\n\nReceived message from process " << received.sender_address << ": \n";
            for (auto byte : received.content) {
                std::cout << byte << std::endl;
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Application has completed. Check performance.log for details." << std::endl;
}

