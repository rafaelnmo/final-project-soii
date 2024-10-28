#include "application.h"
#include "reliable_comm.h"
#include "channels.h"
#include "atomic_broadcast_ring.h"
#include <iostream>

int main(int argc, char** argv) {
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"127.0.0.1", 3000}},
        {1, {"127.0.0.1", 3001}},
        {2, {"127.0.0.1", 3002}}
    };

    //std::string broadcast_type = "UR";
    // Channels channels(nodes);
    // channels.bind_socket(atoi(argv[1]));

    std::string broadcast_type;
    std::cout << "Escolha o tipo de broadcast ('BE', 'UR,' ou 'AB'): ";
    std::cin >> broadcast_type;

    //ReliableComm comm(atoi(argv[1]), nodes, broadcast_type);
    //Application app(&comm);

    ReliableComm comm(atoi(argv[1]), nodes, broadcast_type);
    AtomicBroadcastRing atomic_broadcast(atoi(argv[1]), &comm, { {0, 1}, {1, 2}, {2, 0} });

    Application app(&comm, &atomic_broadcast);
    app.run(atoi(argv[1]), broadcast_type);
    
    return 0;
}
