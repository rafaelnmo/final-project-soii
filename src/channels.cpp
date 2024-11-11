#include "channels.h"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <numeric>
#include <cstdlib>
#include <ctime>

#define MAX_BUFFER_SIZE 1024

namespace {
    int calculate_hash(const std::vector<uint8_t>& content) {
        int hash = 0;
        hash = std::accumulate(content.begin(), content.end(), 0);
        return hash;
    }
}

Channels::Channels(const std::map<int, std::pair<std::string, int>>& nodes, const std::string conf, const int chance, const int delay)
    : nodes(nodes), conf(conf), chance(chance), delay(delay) {
        srand(time(0));
    }

void Channels::bind_socket(int process_id) {
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(nodes.at(process_id).second);

    if (bind(sock, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
}

void Channels::send_message(int id, int process_id, Message msg) {

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(nodes.at(id).second);
    inet_pton(AF_INET, nodes.at(id).first.c_str(), &dest_addr.sin_addr);

    std::string address = (nodes.at(id).first) + std::to_string(nodes.at(id).second);

    std::vector<uint8_t> new_message = msg.serialize();

    int msg_hash = calculate_hash(new_message);
    if (this->conf == "FAILCHECK" || this->conf == "FULL") {
        msg_hash += msg_hash;
    }

    if (this->conf == "DELAY" || this->conf == "FULL"){
        sleep(this->delay*100);
    }

    std::vector<uint8_t> msg_with_hash = new_message;
    msg_with_hash.push_back(msg_hash & 0xFF);
    msg_with_hash.push_back((msg_hash >> 8) & 0xFF);

    if (this->conf == "LOSS" || this->conf == "FULL"){
        int roll = rand()%101;
        if (roll < this->chance) {
            sendto(sock, msg_with_hash.data(), msg_with_hash.size(), 0, (const struct sockaddr *)&dest_addr, sizeof(dest_addr));
        }
    } else if (this->conf == "REGULAR") {
        sendto(sock, msg_with_hash.data(), msg_with_hash.size(), 0, (const struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }
}

std::pair<Message, int> Channels::receive_message() {
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    uint8_t buffer[MAX_BUFFER_SIZE];
    int bytes_received = recvfrom(sock, buffer, MAX_BUFFER_SIZE, 0,
                                  (struct sockaddr *)&src_addr, &addr_len);

    if (bytes_received > 0) {
        std::vector<uint8_t> msg(buffer, buffer + bytes_received - 2);
        int msg_hash = buffer[bytes_received - 2] | (buffer[bytes_received - 1] << 8);
        Message new_message = Message::deserialize(msg);
        //std::pair<Message, int> * result = new std::pair<Message, int>(new_message, msg_hash);
        return std::make_pair(new_message, msg_hash); // Adjust sender_id 
    }

    return std::make_pair(Message(), 0); // Return an empty message in case of failure
}
