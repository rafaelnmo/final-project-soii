#include "keyValueStore.h"
#include <sstream>

KeyValueStore::KeyValueStore(AtomicBroadcastRing* comm) : comm(comm) {}

void KeyValueStore::write(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx);
    store[key] = value;

    // std::string message = "WRITE " + key + " " + value;

    //std::vector<uint8_t> message1 = { 'F', 'i', 'r', 's', 't' };
     // Serialize message
    std::string message = "WRITE " + key + " " + value;
    std::vector<uint8_t> message1(message.begin(), message.end());
    try {
        std::cout << "Performing WRITE operation on key: " << key << std::endl;
        // comm->broadcast(message1, "GroupA");
        comm->send(1, message1);
        std::cout << "--- End operation WRITE ---\n\n" << std::endl;
    } catch (const std::exception& e) {
        // Handle potential broadcast errors
        std::cerr << "Broadcast error: " << e.what() << std::endl;
    }
}

std::string KeyValueStore::read(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    if (store.find(key) != store.end()) {
        std::cout << "Performing READ operation on key: " << key << std::endl;
        std::cout << "--- End operation READ ---\n\n" << std::endl;
        return store[key];
    }
    return "NOT_FOUND";
}

void KeyValueStore::processReceivedMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);

    // Validate and parse message
    std::istringstream iss(message);
    std::string operation, key, value;

    if (!(iss >> operation >> key >> value) || operation != "WRITE") {
        std::cerr << "Invalid message received: " << message << std::endl;
        return;
    }

    // Process WRITE operation
    store[key] = value;
}