#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <cstdint>
#include <cstring>

/*
Possible message types:

1:1
    MSG - Contents of message, only one to not be a control message
    SYN
    ACK
    CLS - Close signal

1:n - ARB
    TKV - Token vote to define who has token
    TKT - Token transfer between peers
    TKN - Token notify, notify all peers that token is being transferred

*/

struct Message {
    std::string sender_address;
    int msg_num;
    char msg_type[3];
    uint8_t control_message;
    std::vector<uint8_t> content;
    
    Message(std::string sender_address, int msg_num, char msg_type[], const std::vector<uint8_t>& msg);

    std::vector<uint8_t> serialize() const;
    static Message deserialize(const std::vector<uint8_t>& data);
};

#endif
