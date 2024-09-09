#include "message.h"

Message::Message(int sender, const std::vector<uint8_t>& msg)
    : sender_id(sender), content(msg) {

}

