#include "atomic_broadcast_ring.h"
#include <iostream>

AtomicBroadcastRing::AtomicBroadcastRing(int id, const std::map<int, std::pair<std::string, int>>& nodes)
    : ReliableComm(id, nodes, "AB") {
    // Determine the next node in the ring
    auto it = nodes.find(id);
    if (++it == nodes.end()) {
        it = nodes.begin();
    }
    next_node_id = it->first;
}

int AtomicBroadcastRing::broadcast(const std::vector<uint8_t>& message) {
    int status = broadcast_ring(message);
    if (status==0) {
        broadcast_ring(std::vector<uint8_t>{'D','E','L'});
    } else {
        broadcast_ring(std::vector<uint8_t>{'N','D','E','L'});
    }
    return status;
}

int AtomicBroadcastRing::broadcast_ring(const std::vector<uint8_t>& message) {
    int attempt_count = 0;
    const int max_attempts = 3; // Máximo de tentativas

    while (attempt_count < max_attempts) {
        log("Tentativa de broadcast #" + std::to_string(attempt_count + 1) + " para o nó " + std::to_string(next_node_id));
        
        log("next_node: " + std::to_string(next_node_id));
        // Tenta enviar a mensagem para o próximo nó
        int result = send(next_node_id, message);

        if (result == 0) {
            log("Mensagem enviada com sucesso para o nó " + std::to_string(next_node_id));
            break;
        } else {
            log("Falha ao enviar mensagem na tentativa #" + std::to_string(attempt_count + 1), "WARNING");
            attempt_count++;
        }
    }

    if (attempt_count==max_attempts) {
        log("Exceeded retry amount", "WARNING");
        return -1;
    }

    attempt_count = 0;

    int status = 0;

    log("waiting for ring to complete", "INFO");
    while (attempt_count < max_attempts) {
        Message msg = receive();
        if (msg.sender_id != -1 && (msg.content==message)) {
            log("Ring completed","INFO");
            break;
        }
        attempt_count++;
    }

    if (attempt_count>=max_attempts) {
        status = -1;
        log("Falha ao entregar a mensagem após " + std::to_string(max_attempts) + " tentativas.", "ERROR");
    }

    return status; 
}

Message AtomicBroadcastRing::deliver() {
    Message msg = receive();

    // Deliver message to application
    log("AB: Delivering message from node "  + std::to_string(msg.sender_id ));

    // Forward the message to the next node in the ring unless it's the sender
    if (msg.sender_id != process_id) {
        send(next_node_id, msg.content);
    }

    int counter = 0;
    int max_tries = 3;

    while (counter < max_tries) {
        log("Waiting signal","INFO");
        Message signal = receive();

        for (auto byte : signal.content) {
                std::cout << byte << std::endl;
            }

        if (msg.sender_id != process_id && (signal.content == std::vector<uint8_t>{'D','E','L'} || signal.content==std::vector<uint8_t>{'N','D','E','L'})) {
            log("RESEND Signal");
            send(next_node_id, signal.content);
            break;
        } else {
            counter++;
            log("Wrong signal","WARNING");
        }
    }

    return msg;
}
