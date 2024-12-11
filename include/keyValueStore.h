#ifndef KEYVALUESTORE_H
#define KEYVALUESTORE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include "atomic_broadcast_ring.h"

class KeyValueStore {
private:
    std::unordered_map<std::string, std::string> store;
    std::mutex mtx;
    //AtomicBroadcastRing& ring;
    AtomicBroadcastRing* comm; // Store reference to ReliableComm


public:
    explicit KeyValueStore(AtomicBroadcastRing* comm);
    void write(const std::string& key, const std::string& value);
    std::string read(const std::string& key);
    void processReceivedMessage(const std::string& message);
};

#endif // KEYVALUESTORE_H
