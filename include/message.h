#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <cstdint>
#include <cstring>

struct Message {
    int sender_id;
    std::vector<uint8_t> content;
    
    Message(int sender, const std::vector<uint8_t>& msg);
    Message();

    std::vector<uint8_t> serialize() const;
    static Message deserialize(const std::vector<uint8_t>& data);
};

#endif
