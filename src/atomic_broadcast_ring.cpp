#include "atomic_broadcast_ring.h"
#include <iostream>
#include <thread>
#include <iostream>
#include <csignal>
#include <csetjmp>
#include <optional>
#include <string>

#define ATOMIC_TIMEOUT 10

jmp_buf jumpBuffer_atomic;

AtomicBroadcastRing::AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes,
    std::string conf, int chance, int delay)
    : ReliableComm(id, nodes, "AB", conf, chance, delay) {
    // Determine the next node in the ring
    
    auto it = nodes.find(id);
    if (++it == nodes.end()) {
        it = nodes.begin();
    }
    next_node_id = it->first;

    if (id == 0) {
        log("Starting ring with node " + std::to_string(next_node_id));
        token = true;
    }

    std::thread deliver_listener(&AtomicBroadcastRing::deliver_thread, this);
    deliver_listener.detach();

    std::thread token_monitor_thread(&AtomicBroadcastRing::token_monitor, this);
    token_monitor_thread.detach();
}

void AtomicBroadcastRing::signalHandler(int signum) {
    std::cout << "Timeout reached! Returning from blocking function.\n";
    longjmp(jumpBuffer_atomic, 1);  // Jump back to the saved state
}

int AtomicBroadcastRing::broadcast(const std::vector<uint8_t>& message) {
    std::unique_lock<std::mutex> lock(mtx_token);
    cv_token.wait(lock, [this] { return token; });

    int status = broadcast_ring(message, 3);
    if (status==0) {
        broadcast_ring(std::vector<uint8_t>{'D','E','L'}, 1);
    } else {
        broadcast_ring(std::vector<uint8_t>{'N','D','E','L'}, 1);
    }

    send_token();

    return status;
}

int AtomicBroadcastRing::broadcast_ring(const std::vector<uint8_t>& message, int max_attempts) {
    int attempt_count = 0;
    int status = 0;

    Message msg;

    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);

    while (attempt_count < max_attempts) {
        if (setjmp(jumpBuffer_atomic) == 0) {
            std::signal(SIGALRM, signalHandler);
            pthread_sigmask(SIG_UNBLOCK, &newmask, nullptr);
            alarm(ATOMIC_TIMEOUT);

            log("Tentativa de broadcast #" + std::to_string(attempt_count + 1) + " para o nó " + std::to_string(next_node_id));
            
            log("next_node: " + std::to_string(next_node_id));
            // Tenta enviar a mensagem para o próximo nó
            channels->send_message(next_node_id, process_id, Message(process_address, msg_num, "MSG", message));

            log("waiting for ring to complete", "INFO");

            msg = receive_single_msg();

            if (msg.msg_num != -1 && (msg.content==message)) {
                log("Ring completed","INFO");
                break;
            }
        } else {
            pthread_sigmask(SIG_SETMASK, &oldmask, nullptr);
            alarm(0);
            log("Attempt " + std::to_string(attempt_count) + " timed out.", "WARNING");
        }

        attempt_count++;
    

        if (attempt_count>=max_attempts) {
            status = -1;
            log("Falha ao entregar a mensagem após " + std::to_string(max_attempts) + " tentativas.", "ERROR");
            break;
        }
    }

    return status;
}

int AtomicBroadcastRing::send_token() {
    std::vector<uint8_t> token_msg = {(unsigned char)next_node_id};

    int status;

    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);

    if (setjmp(jumpBuffer_atomic) == 0) {
        channels->send_message(next_node_id, process_id,
                        Message(process_address, msg_num, "TKT", token_msg));
        log("Token sent", "INFO");

        std::signal(SIGALRM, signalHandler);
        pthread_sigmask(SIG_UNBLOCK, &newmask, nullptr);
        alarm(1);

        Message tkn = receive_single_msg();
        if (tkn.content.front() == token_msg.front()) {
            pthread_sigmask(SIG_SETMASK, &oldmask, nullptr);
            alarm(0);

            log("Token passed succesfully", "INFO");
            status = 0;
        }
    } else {
        pthread_sigmask(SIG_SETMASK, &oldmask, nullptr);
        alarm(0);
        log("Timeout on token passing, token not passed", "WARNING");
        status = -1;
    }

    token = false;

    return status;
}


void AtomicBroadcastRing::deliver_thread() {
    while (true) {
        Message msg = receive_single_msg();

        if (msg.msg_type=="TKT") {
            log("Token being passed", "INFO");
            if (msg.content.front() == process_id) {
                log("Token Acquired", "INFO");
                {
                    std::unique_lock<std::mutex> lock(mtx_token);
                    token = true;
                }
                cv_token.notify_all();
            } else {
                log("Token not acquired", "INFO");
            }
            channels->send_message(next_node_id, process_id, msg);
        } else if (msg.msg_type=="TKV") {
            log("Voting on token", "INFO");
            if (msg.content.front() > process_id) {
                msg.content = std::vector<uint8_t>{(unsigned char)process_id};
            }
            channels->send_message(next_node_id, process_id, msg);
        }

        // Deliver message to application
        log("AB: Storing message from node "  + msg.sender_address);

        // Forward the message to the next node in the ring unless it's the sender
        if (msg.msg_num != process_id) {
            channels->send_message(next_node_id, process_id, msg);
        }

        int counter = 0;
        int max_tries = 3;

        while (counter < max_tries) {
            log("Waiting signal","INFO");
            Message signal = receive_single_msg();

            for (auto byte : signal.content) {
                    std::cout << byte << std::endl;
                }

            if ((msg.sender_address) != process_address && (signal.content == std::vector<uint8_t>{'D','E','L'} || signal.content==std::vector<uint8_t>{'N','D','E','L'})) {
                log("RESEND Signal");
                channels->send_message(next_node_id, process_id, signal);

                std::unique_lock<std::mutex> lock(mtx);
                deliver_queue.push(msg);
                cv.notify_all();

                break;
            } else {
                counter++;
                log("Wrong signal","WARNING");
            }
        }

        Message tkn = receive_single_msg();

        if (tkn.msg_type=="TKT") {
            log("Token being passed", "INFO");
            if (tkn.content.front() == process_id) {
                log("Token Acquired", "INFO");
                {
                    std::unique_lock<std::mutex> lock(mtx_token);
                    token = true;
                }
                cv_token.notify_all();
            } else {
                log("Token not acquired", "INFO");
            }
            channels->send_message(next_node_id, process_id, tkn);
        }
    }

}

Message AtomicBroadcastRing::deliver() {
    Message msg;

    std::unique_lock<std::mutex> lock(mtx_deliver);
    bool status = cv_deliver.wait_for(lock, std::chrono::seconds(5), [this] { return !deliver_queue.empty(); });
    if (status) {
        msg = deliver_queue.front();
        deliver_queue.pop();
    } else {
        msg = Message();
    }

    return msg;
}

void AtomicBroadcastRing::token_monitor() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx_token_monitor);
        bool status = cv_token_monitor.wait_for(lock, std::chrono::seconds(10), [this] { return (token); });
        if (!status) {
            log("Token process presumably dead, starting election", "INFO");
        } else {
            log("Token process still alive", "INFO");
        }
        sleep(20);
    }
}