#include "reliable_comm.h"
#include "channels.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <csignal>
#include <csetjmp>
#include <optional>
#include <string>

#define MAX_BUFFER_SIZE 1024
#define RETRY_COUNTER 3
#define TIMEOUT_TIMER 1

jmp_buf jumpBuffer;

ReliableComm::ReliableComm(int id, const std::map<int, std::pair<std::string, int>>& nodes, std::string broadcast_type,
    std::string conf, int chance, int delay)
    : process_id(id), nodes(nodes), broadcast_type(broadcast_type) {
    process_address = nodes.at(id).first + ":" + std::to_string(nodes.at(id).second);
    msg_num = 0;
    communication_state = Waiting;
    // Initialize Channels
    channels = new Channels(nodes, conf, chance, delay);
    channels->bind_socket(id);

    // Create listener thread
    std::thread listener(&ReliableComm::listen, this);
    listener.detach();
}

int ReliableComm::get_process_id() {
    return process_id;
}

int ReliableComm::send(int id, const std::vector<uint8_t>& message) {
    return send_message(id,"MSG", message);
}

int ReliableComm::send_ctrl(int id, std::string msg_type) {
    return send_message(id, msg_type, std::vector<uint8_t>{' '});
}

int ReliableComm::broadcast(const std::vector<uint8_t>& message) {
    std::vector<int> id_list;

    for (auto const& [key, val] : this->nodes) {
        if (key != process_id) {
            id_list.push_back(key);
        };
    }

    if (broadcast_type == "BE") {
        beb_broadcast(id_list, "MSG", message);
    } else if (broadcast_type == "UR") {
        urb_broadcast(id_list,message);
    }
    return 0;
}

void ReliableComm::signalHandler(int signum) {
    std::cout << "Timeout reached! Returning from blocking function.\n";
    longjmp(jumpBuffer, 1);  // Jump back to the saved state
}

// start handshake
Message ReliableComm::send_syn_and_wait_ack(int id) {

    Message msg = Message(process_address, msg_num, "SYN", std::vector<uint8_t>{' '});

    channels->send_message(id, process_id, msg);    // envia SYN

    Message received = receive_single_msg();    // recebe ACK

    return received;
}

Message ReliableComm::send_contents_and_wait_close(int id, std::string msg_type, const std::vector<uint8_t>& message) {

    Message msg = Message(process_address, msg_num, msg_type, message);

    channels->send_message(id, process_id, msg);    // envia MESSAGE

    Message received = receive_single_msg();     // recebe CLOSE

    return received;
}

int ReliableComm::send_message(int id, std::string msg_type, const std::vector<uint8_t>& message) {
    communication_state = SendSYN;
    bool correct = false;

    Message received;
    int counter = 0;

    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);

    log("send_message");
    while (counter < RETRY_COUNTER && !correct) {
        if (setjmp(jumpBuffer) == 0) {
            counter++;

            std::signal(SIGALRM, signalHandler);
            pthread_sigmask(SIG_UNBLOCK, &newmask, nullptr);
            alarm(TIMEOUT_TIMER);

            received = send_syn_and_wait_ack(id);  // blocking function

            pthread_sigmask(SIG_UNBLOCK, &oldmask, nullptr);
            alarm(0);;  // Cancel the alarm if function completes in time

            // send_syn() is complete.
            if (received.msg_type != "ACK") {
                log("received message other than ACK", "ERROR");
            // } else if (received.sender_address != process_address) {
            //     log("received message not from target", "ERROR");
            } else {
                log("correct ACK","INFO");
                correct = true;
            }
        } else {
            pthread_sigmask(SIG_SETMASK, &oldmask, nullptr);
            alarm(0);
            // If longjmp was called, we handle the timeout here
            std::cout << "Attempt " << counter << " timed out.\n";
            log("Send SYN timed out", "WARNING");
        }
    }

    if (counter >= RETRY_COUNTER) {
        log("Retry limit exceeded for SYN, aborting","WARNING");
        communication_state = Waiting;
        return -1;
    } else {
        log("Send SYN succesfull", "DEBUG");
    }

    correct = false;
    counter = 0;
    communication_state = SendMESSAGE;

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);

    while (counter < RETRY_COUNTER && !correct) {
        if (setjmp(jumpBuffer) == 0) {
            counter++;

            std::signal(SIGALRM, signalHandler);
            pthread_sigmask(SIG_UNBLOCK, &newmask, nullptr);
            alarm(TIMEOUT_TIMER);

            received = send_contents_and_wait_close(id, msg_type, message);
            //log("timeout on send contents and wait close", "WARNING");

            pthread_sigmask(SIG_UNBLOCK, &oldmask, nullptr);
            alarm(0);  // Cancel the alarm if function completes in time

            if (received.msg_type != "CLS") {
                log("received message other than CLOSE","WARNING");
            // } else if (received.sender_address != process_address) {
            //     log("received message not from target", "WARNING");
            } else {
                log("correct CLOSE","INFO");
                correct = true;
            }
        } else {
            pthread_sigmask(SIG_SETMASK, &oldmask, nullptr);
            alarm(0);
            // If longjmp was called, we handle the timeout here
            std::cout << "Attempt " << counter << " timed out.\n";
            log("Send CONTENTS timed out", "WARNING");
        }
    }

    communication_state = Waiting;

    if (counter == RETRY_COUNTER) {
        log("Retry limit exceeded for CONTENTS, aborting","WARNING");
        return -1;
    } else {
        log("Send CONTENTS succesfull", "DEBUG");
    }

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
        || (msg.control_message)) {
            log("Received new message", "DEBUG");
            if (msg.msg_type == "HTB" || msg.msg_type == "HSY") {
                // Heartbeat message
                log("Heartbeat message received", "DEBUG");
                htb_queue.push(msg);
                log("Size of HTB queue: "+std::to_string(htb_queue.size()), "DEBUG");
                cv_htb.notify_all();
            } else {
                log("Normal Message received", "DEBUG");
                mark_delivered(msg_hash);
                message_queue.push(msg);
                cv.notify_all();
            }
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
    Message msg = Message(process_address, msg_num, "ACK", std::vector<uint8_t>{' '});

    channels->send_message(received_sender_id, process_id, msg);

    Message received = receive_single_msg(); // recebe MESSAGE

    return received;
}

std::optional<int> ReliableComm::findKeyByValue(std::string address) {
    for (const auto& [key, val] : this->nodes) {
        if ((val.first + ":" + std::to_string(val.second)) == address) {
            return key;  // Return the key if the value is found
        }
    }
    return std::nullopt;  // Return empty if the value is not found
}

Message ReliableComm::receive() {
    int counter = 0;
    //bool loop = true;
    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);

    Message msg = receive_single_msg();

    int received_sender_id = findKeyByValue(msg.sender_address).value();
    log("received new message");

    while (counter < RETRY_COUNTER) {
        log("checking if syn", "DEBUG" );
        if (msg.msg_type == "SYN") {
            if (setjmp(jumpBuffer) == 0) {
                log("it is syn, loop entered, sending ack", "DEBUG");
                counter++;

                    sigemptyset(&newmask);
                    sigaddset(&newmask, SIGALRM);
                    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);

                    log("Send ack recv msg");
                    msg = send_ack_recv_contents(received_sender_id);

                    pthread_sigmask(SIG_UNBLOCK, &oldmask, nullptr);
                    ualarm(0,0);  // Cancel the alarm if function completes in time

                    if (msg.msg_type == "MSG") {
                        log("Contents received");
                        break;
                    }
            } else {
                pthread_sigmask(SIG_SETMASK, &oldmask, nullptr);
                ualarm(0, 0);
                // If longjmp was called, we handle the timeout here
                std::cout << "Attempt " << counter << " timed out.\n";
                log("Send ACK timed out", "WARNING");
            }
        } else {
                log("not syn, waiting new message to start handshake", "DEBUG");
                msg = receive_single_msg();
            }
        }
    log("Sending CLOSE","DEBUG");

    // if (setjmp(jumpBuffer)==0) {
    //     for (int i; i<RETRY_COUNTER; i++) {
    //         std::signal(SIGALRM, signalHandler);
    //         pthread_sigmask(SIG_UNBLOCK, &newmask, nullptr);
    //         alarm(TIMEOUT_TIMER);

            Message msg_close = Message(process_address, msg_num, "CLS", std::vector<uint8_t>{'C', 'L', 'O', 'S', 'E'});
            channels->send_message(received_sender_id, process_id, msg_close);

    //         pthread_sigmask(SIG_UNBLOCK, &oldmask, nullptr);
    //         alarm(0);  // Cancel the alarm if function completes in time  
    //     }
    // } else {
    //     pthread_sigmask(SIG_UNBLOCK, &oldmask, nullptr);
    //     alarm(0);  // Cancel the alarm if function completes in time  
    //     log("CLOSE sent succesgfully", "DEBUG");
    // }


    log("exiting receive()","DEBUG");
    return msg;
}

int ReliableComm::beb_broadcast(const std::vector<int> id_list, std::string msg_type, const std::vector<uint8_t> message) {
    log("***** Start BE *****");
    int success = 0;
    for (const int& id : id_list) {
        if (msg_type == "MSG") {
            std::min(success, send(id, message));
        } else {
            std::min(success, send_ctrl(id, msg_type));
        }
    }
    log("***** End BE *****" );
    return success;
}

int ReliableComm::urb_broadcast(const std::vector<int> id_list, const std::vector<uint8_t> message) {
    log("***** Start URB *****");
    int status = beb_broadcast(id_list, "MSG", message);

    log("Status: "+ std::to_string(status), "DEBUG");
    log("Send URB signal", "DEBUG");
    if (!status) {
        // Send ACK deliver
        log("DEL", "DEBUG");
        beb_broadcast(id_list, "DEL", std::vector<uint8_t>{' '});
    } else {
        // Send NACK deliver
        log("NDEL","DEBUG");
        beb_broadcast(id_list, "NDL", std::vector<uint8_t>{' '});
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
        
        if (urb_sign.msg_type=="DEL") {
            log("DEL", "DEBUG");
            return msg;
        } else if (urb_sign.msg_type=="NDL") {
            log("NDL", "DEBUG");
            continue;
        }
    }
}

void ReliableComm::log(const std::string& message, const std::string& level) {
    std::cout << "[" << level << "] " << message << std::endl;
}
