#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <cstdint>

struct Message {
    int sender_id;
    std::vector<uint8_t> content;
    
    Message(int sender, const std::vector<uint8_t>& msg);
};

#endif
