#include "atomic_broadcast_ring.h"
#include <iostream>

#define ATMOCI_TIMEOUT 15

AtomicBroadcastRing::AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes)
    : ReliableComm(id, nodes, "AB") {
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
}

// int AtomicBroadcastRing::find_next_node(int id) {
//     auto it = nodes.find(id);
//     if (++it == nodes.end()) {
//         it = nodes.begin();
//     }
//     return it->first;
// }

int AtomicBroadcastRing::broadcast(const std::vector<uint8_t>& message) {
    std::unique_lock<std::mutex> lock(mtx_token);
    cv_token.wait(lock, [this] { return token; });

    int status = broadcast_ring(message);
    if (status==0) {
        broadcast_ring(std::vector<uint8_t>{'D','E','L'});
    } else {
        broadcast_ring(std::vector<uint8_t>{'N','D','E','L'});
    }

    std::vector<uint8_t> token_msg = {(unsigned char)next_node_id};

    channels->send_message(next_node_id, process_id,
                    Message(process_address, msg_num, "TKT", token_msg));
    log("Token sent", "INFO");

    Message tkn = receive_single_msg();
    if (tkn.content.front() == token_msg.front()) {
        log("Token passed seuccesfully", "INFO");
        token = false;
    }

    return status;
}

int AtomicBroadcastRing::broadcast_ring(const std::vector<uint8_t>& message) {
    int attempt_count = 0;
    const int max_attempts = 3; // Máximo de tentativas
    int status = 0;

    while (attempt_count < max_attempts) {
        log("Tentativa de broadcast #" + std::to_string(attempt_count + 1) + " para o nó " + std::to_string(next_node_id));
        
        log("next_node: " + std::to_string(next_node_id));
        // Tenta enviar a mensagem para o próximo nó
        int result = channels->send_message(next_node_id, process_id, Message(process_address, msg_num, "MSG", message));

        if (result != -1) {
            log("Mensagem enviada com sucesso para o nó " + std::to_string(next_node_id));
        } else {
            log("Falha ao enviar mensagem para " + std::to_string(next_node_id) + ",pulando para o próximo nó" , "WARNING");
            attempt_count++;
        }
    

    // if (attempt_count==max_attempts) {
    //     log("Exceeded retry amount", "WARNING");
    //     return -1;
    // }

    // attempt_count = 0;

    // int status = 0;

        log("waiting for ring to complete", "INFO");

        Message msg = receive_single_msg();


        if (msg.msg_num != -1 && (msg.content==message)) {
            log("Ring completed","INFO");
            break;
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


Message AtomicBroadcastRing::deliver() {
    Message msg = receive_single_msg();

    // Deliver message to application
    log("AB: Storing message from node "  + msg.sender_address);

    // Forward the message to the next node in the ring unless it's the sender
    if (msg.msg_num != process_id) {
        channels->send_message(next_node_id, process_id, msg);
    } else {
        return Message();
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

    return msg;
}
