#ifndef CHANNELS_H
#define CHANNELS_H

#include "message.h"
#include <map>
#include <string>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>

/*
Types of conf:
"REGULAR"   : Regular Operation, no failure injection, chance and maxdelay is ignored
"LOSS"      : Random failure on send, use chance to determine what % of messages are dropped
"FAILCHECK" : Sends an incorrect checksum with the message use chance to determine what % of messages have wrong checksum
"FULL"      : uses all failure injection settings at the same time
 */

class Channels {
public:
    Channels(const std::map<int, std::pair<std::string, int>>& nodes, const std::string conf, const int chance, const int delay);
    void bind_socket(int process_id);
    void send_message(int id, int process_id, Message msg);
    std::pair<Message, int> receive_message();

private:
    std::map<int, std::pair<std::string, int>> nodes;
    int sock;
    struct sockaddr_in my_addr;
    std::string conf;
    int chance;
    int delay;
};

#endif