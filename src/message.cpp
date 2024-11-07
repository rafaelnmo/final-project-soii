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
    int sender_address_size = sender_address.size();

    // Reserve enough space for the serialized data
    serialized.resize(sizeof(sender_address_size) + sender_address_size + sizeof(msg_num) + sizeof(control_message) + sizeof(content_size) + content_size);

    uint8_t* ptr = serialized.data();

    std::memcpy(ptr, &sender_address_size, sizeof(sender_address_size));
    ptr += sizeof(sender_address_size);

    std::memcpy(ptr, sender_address.data(), sender_address_size);
    ptr += sender_address_size;

    std::memcpy(ptr, &msg_num, sizeof(msg_num));
    ptr += sizeof(msg_num);

    std::memcpy(ptr, &control_message, sizeof(control_message));
    ptr += sizeof(control_message);

    std::memcpy(ptr, &content_size, sizeof(content_size));
    ptr += sizeof(content_size);

    std::memcpy(ptr, content.data(), content_size);

    return serialized;
}

Message Message::deserialize(const std::vector<uint8_t>& serialized) {
    Message msg;
    const uint8_t* ptr = serialized.data();

    int sender_address_size;
    std::memcpy(&sender_address_size, ptr, sizeof(sender_address_size));
    ptr += sizeof(sender_address_size);

    msg.sender_address.resize(sender_address_size);
    std::memcpy(&msg.sender_address[0], ptr, sender_address_size);
    ptr += sender_address_size;

    std::memcpy(&msg.msg_num, ptr, sizeof(msg.msg_num));
    ptr += sizeof(msg.msg_num);

    std::memcpy(&msg.control_message, ptr, sizeof(msg.control_message));
    ptr += sizeof(msg.control_message);

    int content_size;
    std::memcpy(&content_size, ptr, sizeof(content_size));
    ptr += sizeof(content_size);

    msg.content.resize(content_size);
    std::memcpy(&msg.content[0], ptr, content_size);

    return msg;
}

