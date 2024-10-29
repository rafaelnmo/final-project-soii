#include "application.h"
#include "reliable_comm.h"
#include "atomic_broadcast_ring.h"
#include "channels.h"
#include <iostream>
#include <map>

int main(int argc, char** argv) {
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"127.0.0.1", 3000}},
        {1, {"127.0.0.1", 3001}},
        {2, {"127.0.0.1", 3002}},
        {3, {"127.0.0.1", 3003}}
    };

    std::string broadcast_type = argv[2]; // "BE", "UR", or "AB"
    ReliableComm* comm;

    if (broadcast_type == "AB") {
        std::cout << "----- Executando Atomic Broadcast -----\n";
        comm = new AtomicBroadcastRing(atoi(argv[1]), nodes);
    } else if(broadcast_type == "BE"){
        std::cout << "----- Executando Best Effort Broadcast -----\n";
        comm = new ReliableComm(atoi(argv[1]), nodes, broadcast_type);
    } else if(broadcast_type == "UR"){
        std::cout << "----- Executando Uniform Reliable Broadcast -----\n";
        comm = new ReliableComm(atoi(argv[1]), nodes, broadcast_type);
    }

    Application app(comm);
    app.run(atoi(argv[1]));

    return 0;
}
