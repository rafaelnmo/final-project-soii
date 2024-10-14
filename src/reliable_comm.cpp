#include "reliable_comm.h"
#include "channels.h"
#include "timeout.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <future>

#define MAX_BUFFER_SIZE 1024
#define RETRY_COUNTER 2
#define TIMEOUT_TIMER 1

ReliableComm::ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes, const std::string broadcast_type)
    : process_id(id), nodes(nodes), broadcast_type(broadcast_type) {
    communication_state = Waiting;
    // Initialize Channels
    channels = new Channels(nodes);
    channels->bind_socket(id);

    // Create listener thread
    std::thread listener(&ReliableComm::listen, this);
    listener.detach();
}

int ReliableComm::get_process_id() {
    return process_id;
}

int ReliableComm::send(int id, const std::vector<uint8_t>& message) {
    return send_message(id, message);
}

int ReliableComm::broadcast(const std::vector<uint8_t>& message) {
    std::vector<int> id_list;

    for (auto const& [key, val] : this->nodes) {
        if (key != process_id) {
            id_list.push_back(key);
        };
    }

    if (broadcast_type == "BE") {
        beb_broadcast(id_list,message);
    } else if (broadcast_type == "UR") {
        urb_broadcast(id_list,message);
    }
    return 0;
}

// start handshake
Message ReliableComm::send_syn_and_wait_ack(int id) {
    //std::cout<< "sending syn" << std::endl;
    channels->send_message(id, process_id, std::vector<uint8_t>{'S', 'Y', 'N'});    // envia SYN
    //std::cout<< "syn sent" << std::endl;

    //std::cout<< "waiting for ack" << std::endl;
    Message received = receive_single_msg();    // recebe ACK
    //std::cout<< "ack received" << std::endl;

    return received;
}

Message ReliableComm::send_contents_and_wait_close(int id, const std::vector<uint8_t>& message) {
    //std::cout<< "send contents" << std::endl;
    channels->send_message(id, process_id, message);    // envia MESSAGE
    //std::cout<< "contents received" << std::endl;

    //std::cout<< "receive CLOSE" << std::endl;
    Message received = receive_single_msg();     // recebe CLOSE
    //std::cout<< "CLOSE received" << std::endl;

    return received;
}

int ReliableComm::send_message(int id, const std::vector<uint8_t>& message) {
    communication_state = SendSYN;
    bool loop = true;
    Message received;
    int counter = 0;
    while (loop) {
        try {
            Timeout timeout(TIMEOUT_TIMER);
            timeout.start();
            received =  send_syn_and_wait_ack(id);
        } catch (Timeout * t) {
            std::cout << "Timeout on send syn and wait ack" << std::endl;
            counter++;
            if (counter >= RETRY_COUNTER) {
                std::cout<< "exceeded retry limit, aborting communication" << std::endl;
                return -1;
            }
            continue;
        }

        // send_syn() is complete.
        std::cout<< "no timeout on syn" << std::endl;
        if (received.content != std::vector<uint8_t>{'A', 'C', 'K'}) {
            std::cout<< "received message other than ACK" << std::endl;
            loop = true;
        } else if (received.sender_id != id) {
            std::cout<< "received message not from target" << std::endl;
            loop = true;
        } else {
            loop = false;
        }
    }

    loop = true;
    counter = 0;
    communication_state = SendMESSAGE;

    while (loop) {
        try {
            Timeout timeout(TIMEOUT_TIMER);
            timeout.start();
            received = send_contents_and_wait_close(id, message);
        } catch (Timeout * t) {
            std::cout << "Timeout on send contents and wait close" << std::endl;
            counter++;
            if (counter >= RETRY_COUNTER) {
                std::cout<< "exceeded retry limit, aborting communication" << std::endl;
                return -1;
            }
            continue;
        }

        // send_syn() is complete.
        std::cout<< "no timeout on contents" << std::endl;
        if (received.content != std::vector<uint8_t>{'C', 'L','O','S','E'}) {
            std::cout<< "received message other than CLOSE" << std::endl;
            loop = true;
        } else if (received.sender_id != id) {
            std::cout<< "received message not from target" << std::endl;
            loop = true;
        } else if (counter >= RETRY_COUNTER) {
            std::cout<< "exceeded retry limit, aborting communication" << std::endl;
            return -1;  // failure to send message
        } else {
            loop = false;
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
    //std::cout<< "sending ack" << std::endl;
    channels->send_message(received_sender_id, process_id, std::vector<uint8_t>{'A', 'C', 'K'});
    //std::cout<< "ack sent" << std::endl;

    //std::cout<< "waiting msg" << std::endl;
    Message msg = receive_single_msg(); // recebe MESSAGE
    //std::cout<< "msg received" << std::endl;
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
            
            while (inner_loop) {
                try {
                    Timeout timeout(TIMEOUT_TIMER);
                    timeout.start();
                    msg = send_ack_recv_contents(received_sender_id);
                } catch (Timeout * t) {
                    // send_ack() is not complete.
                    std::cout<< "timeout on msg" << std::endl;
                    loop = true;
                    counter++;
                    if (counter >= RETRY_COUNTER) {
                        std::cout<< "exceeded retry limit, aborting communication" << std::endl;
                        inner_loop = false;
                    }
                }
                
                // send_ack() is complete.
                std::cout<< "no timeout on msg" << std::endl;
                if (msg.sender_id == received_sender_id) {
                    std::cout << "Correct sender id" << std::endl;
                    loop = false;
                    inner_loop = false;
                }
            }

            if (!inner_loop && !loop) {
                std::cout<< "send CLOSE" << std::endl;
                channels->send_message(received_sender_id, process_id, std::vector<uint8_t>{'C', 'L', 'O', 'S', 'E'});
                loop = false;
            }
        } else {
            std::cout<< "not syn, waiting new message to start handshake" << std::endl;
            msg = receive_single_msg();
        }
    }
    std::cout << "exiting receive()" << std::endl;
    return msg;
}

int ReliableComm::beb_broadcast(const std::vector<int> id_list, const std::vector<uint8_t> message) {
    std::cout << "Start best effort *****" << std::endl;
    int success = 0;
    for (const int& id : id_list) {
        std::min(success, send(id, message));
    }
    std::cout << "End best effort *****" << std::endl;
    return success;
}

int ReliableComm::urb_broadcast(const std::vector<int> id_list, const std::vector<uint8_t> message) {
    std::cout << "Start URB *****" << std::endl;
    int status = beb_broadcast(id_list, message);

    std::cout << "Status: "<< status<<" *****" << std::endl;
    std::cout << "Send URB signal *****" << std::endl;

    if (!status) {
        // Send ACK deliver
        std::cout << "DEL *****" << std:: endl;
        beb_broadcast(id_list, std::vector<uint8_t>{'D', 'E', 'L'});
    } else {
        // Send NACK deliver
        std::cout << "NDEL *****" << std:: endl;
        beb_broadcast(id_list, std::vector<uint8_t>{'N','D', 'E', 'L'});
    }

    std::cout << "End URB*****" << std::endl;
    return status;
}

Message ReliableComm::deliver() {
    while (true) {
        Message msg = receive();

        if (broadcast_type == "BE") {
            std::cout << "Deliver best effort *****" << std::endl;
            return msg;
        }

        std::cout << "Wait URB signal *****" << std::endl;

        Message urb_sign = receive();
        
        if (urb_sign.content == std::vector<uint8_t>{'D', 'E', 'L'}) {
            std::cout << "DEL *****" << std::endl;
            return msg;
        } else if (urb_sign.content == std::vector<uint8_t>{'N', 'D', 'E', 'L'}) {
            std::cout << "NDEL *****" << std::endl;
            continue;
        }
    }
}