#ifndef REQ_H
#define REQ_H

#include <string>
#include <vector>
#include <cstdint>


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
    static Request deserialize(const std::vector<uint8_t>& data);
};

#endif