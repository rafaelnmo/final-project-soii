#include "message.h"
#include <iostream>

Message::Message()
    : sender_address("ERROR"), msg_num(-1), msg_type("ERR"), control_message(1), content(std::vector<uint8_t>{}) {}

Message::Message(std::string sender_address, int msg_num, std::string msg_type, const std::vector<uint8_t>& msg)
    : sender_address(sender_address), msg_num(msg_num), msg_type(msg_type), content(msg) {
        if (msg_type=="MSG") {
            control_message = 0;
        } else if ((msg_type=="SYN")
                || (msg_type=="ACK")
                || (msg_type=="CLS")
                || (msg_type=="TKV")
                || (msg_type=="TKT")
                || (msg_type=="TKN") ) {
            control_message = 1;
        } else {
            std::cerr << "Invalid message type" << std::endl;
        }
    }

std::vector<uint8_t> Message::serialize() const {
    std::vector<uint8_t> serialized;
    int sender_address_size = sender_address.size();
    int msg_type_size = msg_type.size();
    int content_size = content.size();

    // Reserve enough space for the serialized data
    serialized.resize(sizeof(sender_address_size) + sender_address_size + sizeof(msg_num) + sizeof(msg_type_size) + msg_type_size + sizeof(control_message) + sizeof(content_size) + content_size);

    uint8_t* ptr = serialized.data();

    std::memcpy(ptr, &sender_address_size, sizeof(sender_address_size));
    ptr += sizeof(sender_address_size);

    std::memcpy(ptr, sender_address.data(), sender_address_size);
    ptr += sender_address_size;

    std::memcpy(ptr, &msg_num, sizeof(msg_num));
    ptr += sizeof(msg_num);

    std::memcpy(ptr, &msg_type_size, sizeof(msg_type_size));
    ptr += sizeof(msg_type_size);

    std::memcpy(ptr, msg_type.data(), msg_type_size);
    ptr += msg_type_size;

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

    int msg_type_size;
    std::memcpy(&msg_type_size, ptr, sizeof(msg_type_size));
    ptr += sizeof(msg_type_size);

    msg.msg_type.resize(msg_type_size);
    std::memcpy(&msg.msg_type[0], ptr, msg_type_size);
    ptr += msg_type_size;

    std::memcpy(&msg.control_message, ptr, sizeof(msg.control_message));
    ptr += sizeof(msg.control_message);

    int content_size;
    std::memcpy(&content_size, ptr, sizeof(content_size));
    ptr += sizeof(content_size);

    msg.content.resize(content_size);
    std::memcpy(&msg.content[0], ptr, content_size);

    return msg;
}