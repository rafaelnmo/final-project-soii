#include "keyValueStore.h"
#include <sstream>

KeyValueStore::KeyValueStore(AtomicBroadcastRing* comm) : comm(comm) {}

void KeyValueStore::write(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx);
    store[key] = value;

    // std::string message = "WRITE " + key + " " + value;

    std::vector<uint8_t> message1 = { 'F', 'i', 'r', 's', 't' };
    comm->broadcast(message1, "GroupA");
}

std::string KeyValueStore::read(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    if (store.find(key) != store.end()) {
        return store[key];
    }
    return "NOT_FOUND";
}

void KeyValueStore::processReceivedMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);

    std::istringstream iss(message);
    std::string operation, key, value;
    iss >> operation >> key >> value;

    if (operation == "WRITE") {
        store[key] = value;
    }
}
