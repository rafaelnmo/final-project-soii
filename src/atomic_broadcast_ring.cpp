#include "atomic_broadcast_ring.h"
#include <iostream>
#include <thread>
#include <iostream>
#include <csignal>
#include <csetjmp>
#include <optional>
#include <string>
#include <cmath>

#define ATOMIC_TIMEOUT 10

jmp_buf jumpBuffer_atomic;

AtomicBroadcastRing::AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes,
    std::string conf, int chance, int delay)
    : ReliableComm(id, nodes, "AB", conf, chance, delay) {

    // Initialize participant states to Uninitialized
    for (const auto& node : nodes) {
        participant_states[node.first] = ParticipantState::Uninitialized;
    }
    participant_states[id] = ParticipantState::Active;

    // initialize other nodes responses to suspect everyone
    for (const auto& node : nodes) {
        witness_of_nodes[node.first] = std::set<uint8_t>{};
    }
    witness_of_nodes[id].insert(id);

    // Determine the next node in the ring
    next_node_id = find_next_node(process_id);

    // set amount of failures
    failures = 1;

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

    std::thread htb_handler(&AtomicBroadcastRing::htb_handler_thread, this);
    htb_handler.detach();
}

int AtomicBroadcastRing::find_next_node(int key) {
    auto it = nodes.find(key);
    if (++it == nodes.end()) {
        it = nodes.begin();
    }
    std::cout << "Next node: " << it->first << std::endl;
    return it->first;
}
    

// Heartbeat message sending logic
void AtomicBroadcastRing::send_heartbeat() {
    while (true) {
        std::vector<uint8_t> heartbeat_msg;
        for (const auto& entry : participant_states) {
            heartbeat_msg.push_back(static_cast<uint8_t>(entry.second));  // Serialize all participants states
        }

        // Send heartbeat to all participants (except self)
        for (const auto& node : nodes) {
            if (node.first != process_id) {
                //log("Sending heartbeat to node " + std::to_string(node.first), "INFO");
                channels->send_message(node.first, process_id, Message(process_address, msg_num, "HTB", heartbeat_msg));
            }
        }

        // Sleep for the heartbeat interval
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval));
    }
}

// Process received heartbeat messages
void AtomicBroadcastRing::process_heartbeat(const Message& msg) {
    //log("Processing heartbeat message", "INFO");

    int key = find_key(msg.sender_address);

    // If the sender is uninitialized, mark it as active
    if (participant_states[key] == ParticipantState::Uninitialized) {
        //log("Node " + std::to_string(key) + " is now active", "INFO");
        participant_states[key] = ParticipantState::Active;
        //witness_of_nodes[key].insert(key);
        witness_of_nodes[key].insert(process_id);
        for (int i=0; i<static_cast<int>(msg.content.size()); i++) {
            if (msg.content[i] == 1) {
                witness_of_nodes[i].insert(key);
            }
        }
        std::vector<uint8_t> heartbeat_msg;
        for (const auto& entry : participant_states) {
            if (entry.first != process_id && entry.second == ParticipantState::Active) {
                heartbeat_msg.push_back(static_cast<uint8_t>(entry.second));  // Serialize all participants states
            }
        }
        channels->send_message(key, process_id, Message(process_address, msg_num, "HTB", heartbeat_msg));
    }
    // else if (participant_states[key] == ParticipantState::Active) {
    //     //log("Node " + std::to_string(key) + " is still active", "INFO");
    // }
}

int AtomicBroadcastRing::find_key(std::string address) {
    for (const auto& [key, val] : nodes) {
        if ((val.first + ":" + std::to_string(val.second)) == address) {
            return key;  // Return the key if the value is found
        }
    }
    return -1;  // Return empty if the value is not found
}

// Detect defective processes based on heartbeats
void AtomicBroadcastRing::detect_defective_processes() {
    send_htb = true;
    while (true) {
        // three steps for a round:
        // 1- collect HTBs for alloted time
        // 2- Make final decision based on the collected information

        // Sleep before checking again
        std::this_thread::sleep_for(std::chrono::milliseconds(5*heartbeat_interval));
        log("Detecting defective processes", "INFO");

        // exchange HSYs
        int amount_alive = 0;
        for (auto&entry :participant_states) {
            if (entry.second == ParticipantState::Active) {
                amount_alive++;
            }
        }

        if (amount_alive <= (2*failures)) {
            //log("Not enough nodes alive alive, no need to exchange HSYs", "INFO");
            print_states();
            for (const auto& node : nodes) {
                participant_states[node.first] = ParticipantState::Uninitialized;
            }
            participant_states[process_id] = ParticipantState::Active;
            // initialize other nodes responses to suspect everyone
            for (const auto& node : nodes) {
                witness_of_nodes[node.first].clear();
            }
            witness_of_nodes[process_id].insert(process_id);
            continue;
        } else {
            for (auto& entry:witness_of_nodes) {
                if (static_cast<int>(entry.second.size()) < (2*failures)+1) {
                    //log("Node " + std::to_string(entry.first) + " is down", "INFO");
                    participant_states[entry.first] = ParticipantState::Uninitialized;
                } else {
                    participant_states[entry.first] = ParticipantState::Active;
                }
            }
        }
        //All failure detection done

        print_states();
        for (const auto& node : nodes) {
            participant_states[node.first] = ParticipantState::Uninitialized;
        }
        participant_states[process_id] = ParticipantState::Active;
        // initialize other nodes responses to suspect everyone
        for (const auto& node : nodes) {
            witness_of_nodes[node.first].clear();
        }
        witness_of_nodes[process_id].insert(process_id);
    }
}

// Buffer messages for uninitialized nodes
// void AtomicBroadcastRing::buffer_message_for_uninitialized(const Message& msg) {
//     if (participant_states[find_key(msg.sender_address)] == ParticipantState::Uninitialized) {
//         message_buffers[find_key(msg.sender_address)].push(msg);
//     }
// }

// Deliver buffered messages when the node becomes active
// void AtomicBroadcastRing::deliver_buffered_messages(int node_id) {
//     if (participant_states[node_id] == ParticipantState::Active) {
//         while (!message_buffers[node_id].empty()) {
//             Message msg = message_buffers[node_id].front();
//             message_buffers[node_id].pop();
//             deliver_queue.push(msg);  // Deliver message to the application
//         }
//     }
// }

// Process incoming messages and handle state changes
void AtomicBroadcastRing::htb_handler_thread() {
    while (true) {
        Message msg;

        std::unique_lock<std::mutex> lock(mtx_htb);
        bool status = cv_htb.wait_for(lock, std::chrono::seconds(5), [this] { return !htb_queue.empty(); });
        if (status) {
            //log("Heartbeat message received", "INFO");
            msg = htb_queue.front();
            htb_queue.pop();

            // Handle receiving a heartbeat message
            process_heartbeat(msg);
        } else {
            log("No heartbeat messages received", "INFO");
        }
    }
}

void AtomicBroadcastRing::print_states() {
    for (const auto& entry : participant_states) {
        std::cout << "Node " << entry.first << ": ";
        switch (entry.second) {
            case ParticipantState::Uninitialized:
                std::cout << "Uninitialized\n";
                break;
            case ParticipantState::Active:
                std::cout << "Active\n";
                break;
            case ParticipantState::Suspicious:
                std::cout << "Suspicious\n";
                break;
            case ParticipantState::Defective:
                std::cout << "Defective\n";
                break;
        }
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