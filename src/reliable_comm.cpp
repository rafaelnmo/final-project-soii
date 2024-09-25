#include "reliable_comm.h"
#include "channels.h"
#include "failure_detection.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <future>

#define MAX_BUFFER_SIZE 1024
#define RETRY_COUNTER 5

ReliableComm::ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes)
    : process_id(id), nodes(nodes) {
    communication_state = Waiting;
    // Initialize Channels and FailureDetection
    channels = new Channels(nodes);
    channels->bind_socket(id);
    //failure_detection = new FailureDetection(5);

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

Message ReliableComm::send_syn_and_wait_ack(int id) {
    std::cout<< "sending syn" << std::endl;
    channels->send_message(id, std::vector<uint8_t>{'S', 'Y', 'N'});    // envia SYN
    std::cout<< "syn sent" << std::endl;

    std::cout<< "waiting for ack" << std::endl;
    Message received = receive_single_msg();    // recebe ACK
    std::cout<< "ack received" << std::endl;

    return received;
}

Message ReliableComm::send_contents_and_wait_close(int id, const std::vector<uint8_t>& message) {
    std::cout<< "send msg" << std::endl;
    channels->send_message(id, message);    // envia MESSAGE
    std::cout<< "msg received" << std::endl;

    std::cout<< "receive CLOSE" << std::endl;
    Message received = receive_single_msg();     // recebe CLOSE
    std::cout<< "CLOSE received" << std::endl;

    return received;
}

int ReliableComm::send_message(int id, const std::vector<uint8_t>& message) {
    std::future_status status;

    communication_state = SendSYN;
    bool loop = true;
    Message received;
    int counter = 0;
    while (loop) {
        std::future<Message> future = std::async(std::launch::async, [this, id]() {
            return send_syn_and_wait_ack(id);
        });

        status = future.wait_for(std::chrono::milliseconds(1000));

        if (status == std::future_status::timeout) {
            // send_syn() is not complete.
            std::cout<< "timeout on syn" << std::endl;
            loop = true;
            counter++;
        } else if (status == std::future_status::ready) {
            // send_syn() is complete.
            std::cout<< "no timeout on syn" << std::endl;
            received = future.get();
            if (received.content != std::vector<uint8_t>{'A', 'C', 'K'}) {
                std::cout<< "received message other than ACK" << std::endl;
                loop = true;
            } else if (counter >= RETRY_COUNTER) {
                std::cout<< "exceeded retry limit, aborting communication" << std::endl;
                return -1;  // failure to send message
            } else {
                loop = false;
            }
        }
    }
    loop = true;
    
    communication_state = SendMESSAGE;

    while (loop) {
        std::future<Message> future = std::async(std::launch::async, [this, id, message]() {
            return send_contents_and_wait_close(id, message);
        });

        status = future.wait_for(std::chrono::milliseconds(1000));

        if (status == std::future_status::timeout) {
            // send_syn() is not complete.
            std::cout<< "timeout on contents" << std::endl;
            loop = true;
            counter++;
        } else if (status == std::future_status::ready) {
            // send_syn() is complete.
            std::cout<< "no timeout on contents" << std::endl;
            received = future.get();
            if (received.content != std::vector<uint8_t>{'C', 'L','O','S','E'}) {
                std::cout<< "received message other than CLOSE" << std::endl;
                loop = true;
            } else if (counter >= RETRY_COUNTER) {
                std::cout<< "exceeded retry limit, aborting communication" << std::endl;
                return -1;  // failure to send message
            } else {
                loop = false;
            }
        }
    }

    communication_state = Waiting;

    std::cout<< "SENT SUCCESFULLY" << std::endl;
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

Message ReliableComm::send_ack_recv_contents(int received_sender_id) {
    std::cout<< "sending ack" << std::endl;
    channels->send_message(received_sender_id, std::vector<uint8_t>{'A', 'C', 'K'});
    std::cout<< "ack sent" << std::endl;

    std::cout<< "waiting msg" << std::endl;
    Message msg = receive_single_msg(); // recebe MESSAGE
    std::cout<< "msg received" << std::endl;

    return msg;
}

Message ReliableComm::receive() {
    int counter = 0;
    bool loop = true;
    bool inner_loop = true;
    Message msg = receive_single_msg();
    int received_sender_id = msg.sender_id;
    std::cout<< "received new message" << std::endl;

    while (loop) {
        std::cout<< "checking if syn" << std::endl;
        if (msg.content == std::vector<uint8_t>{'S', 'Y', 'N'}) {
            std::cout<< "it is syn, loop entered, sending ack" << std::endl;

            std::future_status status;
            
            while (inner_loop) {
                std::future<Message> future = std::async(std::launch::async, [this, received_sender_id]() {
                    return send_ack_recv_contents(received_sender_id);
                });

                status = future.wait_for(std::chrono::milliseconds(1000));

                if (status == std::future_status::timeout) {
                    // send_syn() is not complete.
                    std::cout<< "timeout on msg" << std::endl;
                    loop = true;
                    counter++;
                    if (counter >= RETRY_COUNTER) {
                        std::cout<< "exceeded retry limit, aborting communication" << std::endl;
                        inner_loop = false;
                    } 
                } else if (status == std::future_status::ready) {
                    // send_syn() is complete.
                    std::cout<< "no timeout on msg" << std::endl;
                    msg = future.get();
                    loop = false;
                    inner_loop = false;
                }
            }

            if (!inner_loop && !loop) {
                std::cout<< "send CLOSE" << std::endl;
                channels->send_message(received_sender_id, std::vector<uint8_t>{'C', 'L', 'O', 'S', 'E'});
                loop = false;
            }
        } else {
            std::cout<< "not syn, waiting new message to start handshake" << std::endl;
            msg = receive_single_msg();
        }
    }
    std::cout<< "exiting receive()" << std::endl;
    return msg;
}

Message ReliableComm::deliver() {
    return receive();
}