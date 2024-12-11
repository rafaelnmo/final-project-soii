#include "request.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<uint8_t> Request::serialize() const {
    std::vector<uint8_t> data;

    // Serialize key
    const uint8_t* keyBytes = reinterpret_cast<const uint8_t*>(&key);
    data.insert(data.end(), keyBytes, keyBytes + sizeof(key));

    // Serialize val
    const uint8_t* valBytes = reinterpret_cast<const uint8_t*>(&val);
    data.insert(data.end(), valBytes, valBytes + sizeof(val));

    // Serialize type
    size_t typeSize = type.size();
    const uint8_t* sizeBytes = reinterpret_cast<const uint8_t*>(&typeSize);
    data.insert(data.end(), sizeBytes, sizeBytes + sizeof(typeSize));
    data.insert(data.end(), type.begin(), type.end());

    return data;
}

Request Request::deserialize(const std::vector<uint8_t>& data) {
    size_t offset = 0;

    // Deserialize key
    if (offset + sizeof(int) > data.size())
        throw std::runtime_error("Invalid data for key");
    int key;
    std::memcpy(&key, &data[offset], sizeof(int));
    offset += sizeof(int);

    // Deserialize val
    if (offset + sizeof(int) > data.size())
        throw std::runtime_error("Invalid data for val");
    int val;
    std::memcpy(&val, &data[offset], sizeof(int));
    offset += sizeof(int);

    // Deserialize type size
    if (offset + sizeof(size_t) > data.size())
        throw std::runtime_error("Invalid data for type size");
    size_t typeSize;
    std::memcpy(&typeSize, &data[offset], sizeof(size_t));
    offset += sizeof(size_t);

    // Deserialize type string
    if (offset + typeSize > data.size())
        throw std::runtime_error("Invalid data for type string");
    std::string type(data.begin() + offset, data.begin() + offset + typeSize);

    return Request(key, val, type);
}