#include "application.h"
#include "reliable_comm.h"
#include "channels.h"
#include "failure_detection.h"
#include <iostream>

int main() {
    std::map<int, std::pair<std::string, int>> nodes = {
        {0, {"127.0.0.1", 3000}},
        {1, {"127.0.0.1", 3001}},
        {2, {"127.0.0.1", 3002}}
    };

    Channels channels(nodes);
    channels.bind_socket(0);

    ReliableComm comm(0, nodes);
    Application app(&comm);

    app.run();

    return 0;
}
