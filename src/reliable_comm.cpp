#include "reliable_comm.h"
#include "channels.h"
#include "failure_detection.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#define MAX_BUFFER_SIZE 1024

ReliableComm::ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes)
    : process_id(id), nodes(nodes) {
    communication_state = Waiting;
    // Initialize Channels and FailureDetection
    channels = new Channels(nodes);
    channels->bind_socket(id);
    failure_detection = new FailureDetection(5);

    // Create listener thread
    std::thread listener(&ReliableComm::listen, this);
    listener.detach();
}

int ReliableComm::get_process_id() {
    return process_id;
}

void ReliableComm::send(int id, const std::vector<uint8_t>& message) {
    send_message(id, message);
}

void ReliableComm::broadcast(const std::vector<uint8_t>& message) {
    for (const auto& node : nodes) {
        if (node.first != process_id) {
            send_message(node.first, message);
        }
    }
}

int ReliableComm::send_message(int id, const std::vector<uint8_t>& message) {
    communication_state = SendSYN;
    //Message received;// = new Message(-1,std::vector<uint8_t>{});
    int counter = 0;

    std::cout<< "sending syn" << std::endl;
    channels->send_message(id, std::vector<uint8_t>{'S', 'Y', 'N'});    // envia SYN

    // recebe ACK
    std::cout<< "waiting for ack" << std::endl;
    Message received = receive_single_msg();
    while (received.content != std::vector<uint8_t>{'A', 'C', 'K'} && counter<5) {
        // Caso receba outra mensagem OR timeout
        std::cout<< "resending syn" << std::endl;
        channels->send_message(id, std::vector<uint8_t>{'S', 'Y', 'N'});    // reenvia SYN

        // recebe ACK
        std::cout<< "rewaiting for ack" << std::endl;
        received = receive_single_msg();

        counter++;
    }
    std::cout<< "ack received" << std::endl;
    if (counter==5) {
        std::cout<< "ERROR" << std::endl;
        return -1;
    }    // falha no envio

    communication_state = SendMESSAGE;
    counter = 0;
    std::cout<< "send msg" << std::endl;
    channels->send_message(id, message);    // envia MESSAGE
    std::cout<< "receive CLOSE" << std::endl;
    received = receive_single_msg();     // recebe CLOSE
    while (received.content != std::vector<uint8_t>{'C', 'L', 'O', 'S', 'E'} && counter<5) {
        // Caso receba outra mensagem OR timeout
        std::cout<< "Resend msg" << std::endl;
        channels->send_message(id, message);    // reenvia MESSAGE
        std::cout<< "close rereceived" << std::endl;
        received = receive_single_msg();     // recebe CLOSE
        counter++;
    }
    if (counter==5) {return -1;}    // falha no envio

    communication_state = Waiting;
    std::cout<< "SENT" << std::endl;
    return 0;   // sucesso
}

bool ReliableComm::is_delivered(int msg_hash) {
    return delivered_messages.find(msg_hash) != delivered_messages.end();
}

void ReliableComm::mark_delivered(int msg_hash) {
    delivered_messages.insert(msg_hash);
}

void ReliableComm::listen() {
    while (true) {
        auto [msg, msg_hash] = channels->receive_message();
        
        std::unique_lock<std::mutex> lock(mtx);
        if (!is_delivered(msg_hash)
        || (msg.content == std::vector<uint8_t>{'S', 'Y', 'N'}
        || msg.content == std::vector<uint8_t>{'A', 'C', 'K'}
        || msg.content == std::vector<uint8_t>{'C', 'L','O','S','E'})) {
            mark_delivered(msg_hash);
            message_queue.push(msg);
            cv.notify_all();
        }
    }
}

Message ReliableComm::receive_single_msg() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !message_queue.empty(); });

    Message msg = message_queue.front();
    message_queue.pop();
    return msg;
}

Message ReliableComm::receive() {
    Message msg = receive_single_msg();
    int received_sender_id = msg.sender_id;
    std::cout<< "received syn" << std::endl;

    bool loop = true;
    // Esta execução inicia ao receber SYN
    //int received_sender_id = msg.sender_id;
    std::cout<< "checking syn" << std::endl;
    if (msg.content == std::vector<uint8_t>{'S', 'Y', 'N'}) {
        while (loop) {
            std::cout<< "loop entered, sending ack" << std::endl;
            channels->send_message(received_sender_id, std::vector<uint8_t>{'A', 'C', 'K'});
            std::cout<< "ack sent" << std::endl;
            // int counter = 0;
            // while (counter<5) {
            //     std::cout<< "rewaiting for SYN" << std::endl;
            //     msg = receive_single_msg(); // recebe SYN
            //     std::cout<< "message received, " << std::endl;
            //     channels->send_message(received_sender_id, std::vector<uint8_t>{'A', 'C', 'K'});    // reenvia ACK
            //     counter++;
            // }
            // if (counter==5) {break;}    // falha

            //counter = 0;
            std::cout<< "receive msg" << std::endl;
            msg = receive_single_msg(); // recebe MESSAGE
            std::cout<< "send CLOSE" << std::endl;
            channels->send_message(received_sender_id, std::vector<uint8_t>{'C', 'L', 'O', 'S', 'E'});
            loop = false;
        }
    }
    std::cout<< "exiting receive()" << std::endl;
    return msg;
}

Message ReliableComm::deliver() {
    return receive();
}