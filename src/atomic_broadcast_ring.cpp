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
    

    // Initialize participant states to Uninitialized
    for (const auto& node : nodes) {
        participant_states[node.first] = ParticipantState::Uninitialized;
    }

    auto it = nodes.find(id);
    if (++it == nodes.end()) {
        it = nodes.begin();
    }
    next_node_id = it->first;

    if (id == 0) {
        log("Starting ring with node " + std::to_string(next_node_id));
        token = true;
    }

    // Start the heartbeat and defect detection threads
    std::thread heartbeat_thread(&AtomicBroadcastRing::send_heartbeat, this);
    heartbeat_thread.detach();

    std::thread defect_detection_thread(&AtomicBroadcastRing::detect_defective_processes, this);
    defect_detection_thread.detach();

    std::thread deliver_listener(&AtomicBroadcastRing::deliver_thread, this);
    deliver_listener.detach();

    std::thread token_monitor_thread(&AtomicBroadcastRing::token_monitor, this);
    token_monitor_thread.detach();
}

// Heartbeat message sending logic
void AtomicBroadcastRing::send_heartbeat() {
    while (true) {
        std::vector<uint8_t> heartbeat_msg;
        for (const auto& entry : participant_states) {
            heartbeat_msg.push_back(static_cast<uint8_t>(entry.second));  // Serialize participant state
        }

        // Send heartbeat to all participants (except self)
        for (const auto& node : nodes) {
            if (node.first != process_id) {
                channels->send_message(node.first, process_id, Message(process_address, msg_num, "HEARTBEAT", heartbeat_msg));
            }
        }

        // Sleep for the heartbeat interval
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval));
    }
}

// Process received heartbeat messages
void AtomicBroadcastRing::process_heartbeat(const Message& msg) {
    std::vector<uint8_t> heartbeat_msg = msg.content;
    int i = 0;
    for (auto& entry : participant_states) {
        entry.second = static_cast<ParticipantState>(heartbeat_msg[i]);
        i++;
    }

    // If the sender is uninitialized, mark it as active
    if (participant_states[msg.sender_address] == ParticipantState::Uninitialized) {
        participant_states[msg.sender_address] = ParticipantState::Active;
    }
}

// Detect defective processes based on heartbeats
void AtomicBroadcastRing::detect_defective_processes() {
    while (true) {
        // Check the heartbeat statuses of participants
        for (auto& entry : participant_states) {
            int suspicious_count = 0;

            // If the participant is suspicious, increment the counter
            if (entry.second == ParticipantState::Suspicious) {
                suspicious_count++;
            }

            // If more than half of the processes suspect this participant, mark it as defective
            if (suspicious_count > (participant_states.size() / 2)) {
                entry.second = ParticipantState::Defective;
            }
        }

        // Sleep before checking again
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval));
    }
}

// Buffer messages for uninitialized nodes
void AtomicBroadcastRing::buffer_message_for_uninitialized(const Message& msg) {
    if (participant_states[msg.receiver_address] == ParticipantState::Uninitialized) {
        message_buffers[msg.receiver_address].push(msg);
    }
}

// Deliver buffered messages when the node becomes active
void AtomicBroadcastRing::deliver_buffered_messages(int node_id) {
    if (participant_states[node_id] == ParticipantState::Active) {
        while (!message_buffers[node_id].empty()) {
            Message msg = message_buffers[node_id].front();
            message_buffers[node_id].pop();
            deliver_message(msg);  // Deliver message to the application
        }
    }
}

// Update the broadcast logic to handle uninitialized states and buffered messages
int AtomicBroadcastRing::broadcast(const std::vector<uint8_t>& message) {
    std::unique_lock<std::mutex> lock(mtx_token);
    cv_token.wait(lock, [this] { return token; });

    // Send message to the ring
    int status = broadcast_ring(message, 3);
    if (status == 0) {
        broadcast_ring(std::vector<uint8_t>{'D', 'E', 'L'}, 1);  // Delivery signal
    } else {
        broadcast_ring(std::vector<uint8_t>{'N', 'D', 'E', 'L'}, 1);  // Negative delivery signal
    }

    send_token();  // Pass token to the next node

    return status;
}

// Process incoming messages and handle state changes
void AtomicBroadcastRing::deliver_thread() {
    while (true) {
        Message msg = receive_single_msg();

        if (msg.msg_type == "HEARTBEAT") {
            process_heartbeat(msg);  // Process the heartbeat message
        }

        // Buffer messages for uninitialized processes
        buffer_message_for_uninitialized(msg);

        // Deliver buffered messages when the process is active
        deliver_buffered_messages(msg.receiver_address);

        // Further message processing...
    }
}

// Token monitoring logic (checking if the token is alive)
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

// Signal handler (e.g., for timeouts)
void AtomicBroadcastRing::signalHandler(int signum) {
    std::cout << "Timeout reached! Returning from blocking function.\n";
    longjmp(jumpBuffer_atomic, 1);
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