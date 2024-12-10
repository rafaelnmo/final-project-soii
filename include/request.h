#ifndef REQ_H
#define REQ_H

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>


/*
Types of request:
    SET - Set a value
    GET - Get a value
    YES - Successfull operation
    NOO - Unsuccessfull operation
*/

struct Request
{
    int key;
    int val;
    std::string type;

    Request(int key, int val, std::string type) : key(key), val(val), type(type) {}
    
    std::vector<uint8_t> serialize() const;
    static Request deserialize(const std::vector<uint8_t>& data) {
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
};

#endif