#ifndef CHANNELS_H
#define CHANNELS_H

#include "message.h"
#include <map>
#include <string>
#include <arpa/inet.h>

class Channels {
public:
    Channels(const std::map<int, std::pair<std::string, int>>& nodes, const std::string conf);
    void bind_socket(int process_id);
    void send_message(int id, int process_id, Message msg);
    std::pair<Message, int> receive_message();

private:
    std::map<int, std::pair<std::string, int>> nodes;
    int sock;
    struct sockaddr_in my_addr;
    std::string conf;
};

#endif