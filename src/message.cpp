#include "message.h"
#include <iostream>

Message::Message(std::string sender_address, int msg_num, char msg_type[], const std::vector<uint8_t>& msg)
    : sender_address(sender_address), msg_num(msg_num), control_message(control_message), content(msg) {
        if (std::strcmp(msg_type, "MSG") == 0) {
            control_message = 0;
        } else if ( (std::strcmp(msg_type, "SYN") == 0)
                || (std::strcmp(msg_type, "ACK") == 0)
                || (std::strcmp(msg_type, "CLS") == 0)
                || (std::strcmp(msg_type, "TKV") == 0)
                || (std::strcmp(msg_type, "TKT") == 0)
                || (std::strcmp(msg_type, "TKN") == 0) ) {
            control_message = 6;
        } else {
            std::cerr << "Invalid message type" << std::endl;
        }
    }

std::vector<uint8_t> Message::serialize() const {
    std::vector<uint8_t> serialized;
    int content_size = content.size();
    // Reserve enough space for the serialized data
    serialized.resize(sizeof(sender_id) + sizeof(content_size) + content_size);

    std::memcpy(serialized.data(), &sender_id, sizeof(sender_id));
    std::memcpy(serialized.data() + sizeof(sender_id), &content_size, sizeof(content_size));
    std::memcpy(serialized.data() + sizeof(sender_id) + sizeof(content_size), content.data(), content_size);

    return serialized;
}

Message Message::deserialize(const std::vector<uint8_t>& serialized) {
    Message msg;
    int content_size;
    
    std::memcpy(&msg.sender_id, serialized.data(), sizeof(msg.sender_id));    
    std::memcpy(&content_size, serialized.data() + sizeof(msg.sender_id), sizeof(content_size));
    
    msg.content = std::vector<uint8_t>(serialized.begin() + sizeof(msg.sender_id) + sizeof(content_size),
                                       serialized.begin() + sizeof(msg.sender_id) + sizeof(content_size) + content_size);
    
    return msg;
}

