#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <cstdint>
#include <cstring>

struct Message {
    int sender_id;
    int msg_num;
    int sequence_number;
    uint8_t control_message;
    std::vector<uint8_t> content;
    
    Message(int sender, int msg_num, int sequence_number, uint8_t control_message,const std::vector<uint8_t>& msg);
    Message();

    std::vector<uint8_t> serialize() const;
    static Message deserialize(const std::vector<uint8_t>& data);
};

#endif
