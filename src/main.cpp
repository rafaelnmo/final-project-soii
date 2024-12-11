#include "application.h"
#include "reliable_comm.h"
#include "atomic_broadcast_ring.h"
#include "channels.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <algorithm>

// Helper function to trim leading and trailing spaces
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    size_t last = str.find_last_not_of(" \t");
    return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, (last - first + 1));
}

std::pair<std::map<int, std::pair<std::string, int>>, std::string> parseConfig(const std::string& config_path) {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        throw std::runtime_error("Failed to open configuration file: " + config_path);
    }

    std::map<int, std::pair<std::string, int>> nodes;
    std::string broadcast_type;
    std::string line;

    while (std::getline(config_file, line)) {
        std::istringstream iss(line);
        std::string key, value;

        if (line.find("nodes") != std::string::npos) {
            // Extracting the part that contains nodes inside the braces
            size_t start = line.find("{{") + 1;
            size_t end = line.find("}}");

            std::string nodes_data = line.substr(start, end - start);
            nodes_data = trim(nodes_data);  // Trim spaces

            // Parsing each node entry
            std::istringstream nodes_stream(nodes_data);
            std::string node_entry;
            while (std::getline(nodes_stream, node_entry, '}')) {
                if (node_entry.find('{') != std::string::npos) {
                    node_entry = node_entry.substr(node_entry.find('{') + 1); // Remove the '{' part
                    node_entry = trim(node_entry);  // Trim spaces

                    // Parse node details: ID, address, and port
                    std::istringstream node_iss(node_entry);
                    int node_id;
                    char sep;
                    std::string address;
                    int port;

                    node_iss >> node_id >> sep;  // Read the node ID and ignore the comma
                    std::getline(node_iss, address, ':');  // Read address before ':'
                    node_iss >> port;  // Read port

                    // Add node to the map
                    nodes[node_id] = {address, port};
                }
            }
        } else if (line.find("broadcast") != std::string::npos) {
            size_t start = line.find('=') + 1;
            size_t end = line.find(';');
            broadcast_type = line.substr(start, end - start);
            broadcast_type = trim(broadcast_type);  // Trim spaces around broadcast type
        }
    }

    config_file.close();

    // Debug prints to check parsed values
    // std::cout << "Parsed broadcast type: " << broadcast_type << std::endl;
    // std::cout << "Parsed nodes: \n";
    // for (const auto& node : nodes) {
    //     std::cout << "Node ID: " << node.first << ", Address: " << node.second.first << ", Port: " << node.second.second << std::endl;
    // }

    return {nodes, broadcast_type};
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <node_id> <config_path>\n";
        return 1;
    }

    int node_id = atoi(argv[1]);
    std::string config_path = argv[2];

    std::map<int, std::pair<std::string, int>> nodes;
    std::string broadcast_type;

    try {
        auto config = parseConfig(config_path);
        nodes = config.first;
        broadcast_type = config.second;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        auto config = parseConfig(std::string("../config.txt"));
        nodes = config.first;
        broadcast_type = config.second;
    }

    // Debug prints to check what values are passed
    std::cout << "Node ID: " << node_id << "\n";
    std::cout << "Broadcast Type: " << broadcast_type << "\n\n";

    std::string conf = "FULL";
    int chance = 0;
    int delay = 1;

    //ReliableComm* comm;
    AtomicBroadcastRing* comm;


    if (broadcast_type == "AB") {
        std::cout << "----- Executando Atomic Broadcast -----\n";
        comm = new AtomicBroadcastRing(node_id, nodes, conf, chance, delay);
    } else if (broadcast_type == "BE") {
        //std::cout << "----- Executando Best Effort Broadcast -----\n";
        //comm = new ReliableComm(node_id, nodes, broadcast_type, conf, chance, delay);
    } else if (broadcast_type == "UR") {
        std::cout << "----- Executando Uniform Reliable Broadcast -----\n";
        //comm = new ReliableComm(node_id, nodes, broadcast_type, conf, chance, delay);
    } else {
        std::cerr << "Error: Unsupported broadcast type: " << broadcast_type << "\n";
        return 1;
    }

    Application app(comm);
    app.run(node_id);

    return 0;
}
