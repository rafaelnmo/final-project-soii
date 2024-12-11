
#include "application.h"
#include <iostream>
#include <stdexcept>

Application::Application(AtomicBroadcastRing* comm)
    : comm(comm), kvStore(nullptr), logger(nullptr),
      monitor(nullptr), loadGen(nullptr) {}

void Application::initialize(int process_id) {

    // Initialize KeyValueStore
    kvStore = new KeyValueStore(comm);

    // Initialize Logger
    logger = new Logger("performance.log");
    logger->log("\nApplication initialized");

    // Initialize Performance Monitor
    monitor = new PerformanceMonitor(logger);

    // Set up Load Generator parameters
    int readWriteRatio = 50; // 70% writes, 30% reads
    int totalOperations = 100;
    int numClients = 5;

    // Initialize Load Generator
    loadGen = new LoadGenerator(readWriteRatio, totalOperations, numClients, *kvStore, *monitor);
}

void Application::run(int process_id) {
    initialize(process_id);

    if (process_id==0) {

         logger->log("Starting performance monitoring");
        monitor->startMonitoring();

        logger->log("Starting load generation");
        std::cout << "Starting load generation..." << std::endl;
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

