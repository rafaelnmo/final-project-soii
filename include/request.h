#ifndef REQ_H
#define REQ_H

#include <string>
#include <vector>

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