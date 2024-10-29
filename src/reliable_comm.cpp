#include "reliable_comm.h"
#include "channels.h"
#include "timeout.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#define MAX_BUFFER_SIZE 1024
#define RETRY_COUNTER 2
#define TIMEOUT_TIMER 1

ReliableComm::ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes, const std::string broadcast_type)
    : process_id(id), nodes(nodes), broadcast_type(broadcast_type) {
    msg_num = 0;
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

    channels->send_message(id, process_id, msg_num, 0, 1, std::vector<uint8_t>{'S', 'Y', 'N'});    // envia SYN

    Message received = receive_single_msg();    // recebe ACK

    return received;
}

Message ReliableComm::send_contents_and_wait_close(int id, const std::vector<uint8_t>& message) {
    channels->send_message(id, process_id, msg_num, 0, 0, message);    // envia MESSAGE

    Message received = receive_single_msg();     // recebe CLOSE

    return received;
}

int ReliableComm::send_message(int id, const std::vector<uint8_t>& message) {
    communication_state = SendSYN;
    bool loop = true;
    Message received;
    int counter = 0;

    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);

    log("send_message");
    while (loop) {
        try {
            Timeout timeout(TIMEOUT_TIMER);
            timeout.start();
            received =  send_syn_and_wait_ack(id);
        } catch (Timeout * t) {
            log("timeout on send syn and wait ack", "WARNING");
            counter++;
            if (counter >= RETRY_COUNTER) {
                log("exceeded retry limit, aborting communication", "ERROR");
                return -1;
            }
            continue;
        }

        // send_syn() is complete.
        log("no timeout on syn", "DEBUG");
        if (received.content != std::vector<uint8_t>{'A', 'C', 'K'}) {
            log("received message other than ACK", "ERROR");
            loop = true;
        } else if (received.sender_id != id) {
            log("received message not from target", "ERROR");
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
            log("timeout on send contents and wait close", "WARNING");
            counter++;
            if (counter >= RETRY_COUNTER) {
                log("exceeded retry limit, aborting communication", "ERROR");
                return -1;
            }
            continue;
        }

        // send_syn() is complete.
        log("no timeout on contents", "DEBUG");
        if (received.content != std::vector<uint8_t>{'C', 'L','O','S','E'}) {
            log("received message other than CLOSE","WARNING");
            loop = true;
        } else if (received.sender_id != id) {
            log("received message not from target", "WARNING");
            loop = true;
        } else if (counter >= RETRY_COUNTER) {
            log( "exceeded retry limit, aborting communication", "ERROR");
            return -1;  // failure to send message
        } else {
            loop = false;
        }
    
    }

    communication_state = Waiting;

    log("SENT SUCCESFULLY");
    msg_num++;
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
    channels->send_message(received_sender_id, process_id, msg_num,0, 1, std::vector<uint8_t>{'A', 'C', 'K'});

    Message msg = receive_single_msg(); // recebe MESSAGE
    return msg;
}

Message ReliableComm::receive() {
    int counter = 0;
    bool loop = true;
    bool inner_loop = true;

    Message msg = receive_single_msg();
    int received_sender_id = msg.sender_id;
    log("received new message");

    while (loop) {
        log("checking if syn", "DEBUG" );
        if (msg.content == std::vector<uint8_t>{'S', 'Y', 'N'}) {
            log("it is syn, loop entered, sending ack", "DEBUG");
            while (inner_loop) {
                try {
                    Timeout timeout(TIMEOUT_TIMER);
                    timeout.start();
                    msg = send_ack_recv_contents(received_sender_id);
                } catch (Timeout * t) {
                    // send_ack() is not complete.
                    log("timeout on msg", "WARNING");
                    loop = true;
                    counter++;
                    if (counter >= RETRY_COUNTER) {
                        log("exceeded retry limit, aborting communication", "ERROR");
                        inner_loop = false;
                    }
                }
                
                // send_ack() is complete.
                log("no timeout on msg", "DEBUG");
                if (msg.sender_id == received_sender_id) {
                    log("correct sender id", "DEBUG");
                    loop = false;
                    inner_loop = false;
                }
            }

            if (!inner_loop && !loop) {
                log("send CLOSE", "DEBUG");
                channels->send_message(received_sender_id, process_id, msg_num, 0, true, std::vector<uint8_t>{'C', 'L', 'O', 'S', 'E'});
                loop = false;
            }
        } else {
            log("not syn, waiting new message to start handshake", "DEBUG");
            msg = receive_single_msg();
        }
    }
    log("exiting receive()","DEBUG");
    return msg;
}

int ReliableComm::beb_broadcast(const std::vector<int> id_list, const std::vector<uint8_t> message) {
    log("***** Start BE *****");
    int success = 0;
    for (const int& id : id_list) {
        std::min(success, send(id, message));
    }
    log("***** End BE *****" );
    return success;
}

int ReliableComm::urb_broadcast(const std::vector<int> id_list, const std::vector<uint8_t> message) {
    log("***** Start URB *****");
    int status = beb_broadcast(id_list, message);

    log("Status: "+ std::to_string(status), "DEBUG");
    log("Send URB signal", "DEBUG");
    if (!status) {
        // Send ACK deliver
        log("DEL", "DEBUG");
        beb_broadcast(id_list, std::vector<uint8_t>{'D', 'E', 'L'});
    } else {
        // Send NACK deliver
        log("NDEL","DEBUG");
        beb_broadcast(id_list, std::vector<uint8_t>{'N','D', 'E', 'L'});
    }

    log("***** End URB *****");
    return status;
}

Message ReliableComm::deliver() {
    while (true) {
        Message msg = receive();

        if (broadcast_type == "BE") {
            log("Deliver BE", "DEBUG");
            return msg;
        }

        log("Wait URB signal", "DEBUG");

        Message urb_sign = receive();
        
        if (urb_sign.content == std::vector<uint8_t>{'D', 'E', 'L'}) {
            log("DEL", "DEBUG");
            return msg;
        } else if (urb_sign.content == std::vector<uint8_t>{'N', 'D', 'E', 'L'}) {
            log("NDEL", "DEBUG");
            continue;
        }
    }
}

void ReliableComm::log(const std::string& message, const std::string& level) {
    std::cout << "[" << level << "] " << message << std::endl;
}
