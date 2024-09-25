#include "message.h"

Message::Message(int sender, const std::vector<uint8_t>& msg)
    : sender_id(sender), content(msg) {
    sender_id = sender;
    content = msg;
}

Message::Message() {
    sender_id = -1;
    content = std::vector<uint8_t>{};
}

