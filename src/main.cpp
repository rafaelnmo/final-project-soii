#include "application.h"
#include "reliable_comm.h"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
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


// std::tuple<
//     std::map<int, std::pair<std::string, int>>, 
//     std::string, 
//     std::map<std::string, std::set<int>>
// > parseConfig(const std::string& config_path) {
//     std::ifstream config_file(config_path);
//     if (!config_file.is_open()) {
//         throw std::runtime_error("Failed to open configuration file: " + config_path);
//     }

//     std::map<int, std::pair<std::string, int>> nodes;
//     std::string broadcast_type;
//     std::map<std::string, std::set<int>> groups;
//     std::string line;

//     while (std::getline(config_file, line)) {
//         std::istringstream iss(line);

//         if (line.find("nodes") != std::string::npos) {
//             // Extracting nodes
//             size_t start = line.find("{{") + 1;
//             size_t end = line.find("}}");
//             std::string nodes_data = line.substr(start, end - start);
//             nodes_data = trim(nodes_data);  // Trim spaces

//             std::istringstream nodes_stream(nodes_data);
//             std::string node_entry;
//             while (std::getline(nodes_stream, node_entry, '}')) {
//                 if (node_entry.find('{') != std::string::npos) {
//                     node_entry = node_entry.substr(node_entry.find('{') + 1); // Remove '{'
//                     node_entry = trim(node_entry);  // Trim spaces

//                     std::istringstream node_iss(node_entry);
//                     int node_id;
//                     char sep;
//                     std::string address;
//                     int port;

//                     node_iss >> node_id >> sep;  // Read node ID
//                     std::getline(node_iss, address, ':');  // Read address
//                     node_iss >> port;  // Read port

//                     nodes[node_id] = {address, port};
//                 }
//             }
//         } else if (line.find("broadcast") != std::string::npos) {
//             // Extracting broadcast type
//             size_t start = line.find('=') + 1;
//             size_t end = line.find(';');
//             broadcast_type = line.substr(start, end - start);
//             broadcast_type = trim(broadcast_type);
//         } else if (line.find("groups") != std::string::npos) {
//             // Extracting groups
//             size_t start = line.find("{{") + 1;
//             size_t end = line.find("}}");
//             std::string groups_data = line.substr(start, end - start);
//             groups_data = trim(groups_data);

//             std::istringstream groups_stream(groups_data);
//             std::string group_entry;
//             while (std::getline(groups_stream, group_entry, '}')) {
//                 if (group_entry.find('{') != std::string::npos) {
//                     group_entry = group_entry.substr(group_entry.find('{') + 1); // Remove '{'
//                     group_entry = trim(group_entry);  // Trim spaces

//                     size_t colon_pos = group_entry.find(':');
//                     std::string group_name = group_entry.substr(0, colon_pos);  // Extract group name
//                     group_name = trim(group_name);

//                     std::string members_data = group_entry.substr(colon_pos + 1);  // Extract members
//                     members_data = trim(members_data);
//                     members_data = members_data.substr(1, members_data.size() - 2);  // Remove outer {}

//                     std::istringstream members_stream(members_data);
//                     std::set<int> members;
//                     std::string member;
//                     while (std::getline(members_stream, member, ',')) {
//                         members.insert(std::stoi(trim(member)));
//                     }

//                     groups[group_name] = members;
//                 }
//             }
//         }
//     }

//     config_file.close();

//     return {nodes, broadcast_type, groups};
// }



int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <node_id> <config_path>\n";
        return 1;
    }

    int node_id = atoi(argv[1]);
    std::string config_path = argv[2];

    std::map<int, std::pair<std::string, int>> nodes;
    std::string broadcast_type;
    // std::map<std::string, std::set<int>> groups;

    // try {
    //     auto [parsed_nodes, parsed_broadcast_type, parsed_groups] = parseConfig(config_path);
    //     nodes = parsed_nodes;
    //     broadcast_type = parsed_broadcast_type;
    //     groups = parsed_groups;
    // } catch (const std::exception& e) {
    //     std::cerr << "Error: " << e.what() << "\n";

    //     // Fallback to default configuration
    //     try {
    //         auto [parsed_nodes, parsed_broadcast_type, parsed_groups] = parseConfig("../config.txt");
    //         nodes = parsed_nodes;
    //         broadcast_type = parsed_broadcast_type;
    //         groups = parsed_groups;
    //     } catch (const std::exception& e) {
    //         std::cerr << "Error loading fallback configuration: " << e.what() << "\n";
    //         return 1;
    //     }
    // }

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

// -----------------------------------------------------------------------
    // if (argc < 2) {
    //     std::cerr << "Usage: " << argv[0] << " <node_id>" << std::endl;
    //     return 1;
    // }

    // int process_id = std::stoi(argv[1]);

    // Define nodes and groups
    // std::map<int, std::pair<std::string, int>> nodes = {
    //     {0, {"127.0.0.1", 3000}},
    //     {1, {"127.0.0.1", 3001}},
    //     {2, {"127.0.0.1", 3002}},
    //     {3, {"127.0.0.1", 3003}}
    // };

    std::map<std::string, std::set<int>> groups = {
        {"GroupA", {0, 1}},
        {"GroupB", {1, 2}},
        {"GroupC", {0, 3}}
    };

    // Debug prints to check what values are passed
    std::cout << "Node ID: " << node_id << "\n";
    std::cout << "Broadcast Type: " << broadcast_type << "\n\n";

    std::cout << "\nNodes:\n";
    for (const auto& node : nodes) {
        std::cout << "Key: " << node.first
                  << ", Value: (" << node.second.first << ", " << node.second.second << ")\n";
    }

    std::cout << "\nGroups:\n";
    for (const auto& group : groups) {
        std::cout << "Group: " << group.first << ", Members: ";
        for (int member : group.second) {
            std::cout << member << " ";
        }
        std::cout << "\n";
    }

    std::string conf = "FULL";
    int chance = 0;
    int delay = 1;

    // Instantiate ReliableComm
    AtomicBroadcastRing* comm;

    if (broadcast_type == "AB") {
        std::cout << "\n----- Executando Atomic Broadcast -----\n";
        comm = new AtomicBroadcastRing(node_id, nodes, conf, chance, delay, groups);
    } else if (broadcast_type == "BE") {
        std::cout << "----- Executando Best Effort Broadcast -----\n";
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

// int main(int argc, char** argv) {
//     if (argc < 3) {
//         std::cerr << "Usage: " << argv[0] << " <node_id> <config_path>\n";
//         return 1;
//     }

//     int node_id = atoi(argv[1]);
//     std::string config_path = argv[2];

//     std::map<int, std::pair<std::string, int>> nodes;
//     std::string broadcast_type;

//     try {
//         auto config = parseConfig(config_path);
//         nodes = config.first;
//         broadcast_type = config.second;
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//         auto config = parseConfig(std::string("../config.txt"));
//         nodes = config.first;
//         broadcast_type = config.second;
//     }

//     // Debug prints to check what values are passed
//     std::cout << "Node ID: " << node_id << "\n";
//     std::cout << "Broadcast Type: " << broadcast_type << "\n\n";

//     std::string conf = "FULL";
//     int chance = 0;
//     int delay = 1;

//     ReliableComm* comm;

//     if (broadcast_type == "AB") {
//         std::cout << "----- Executando Atomic Broadcast -----\n";
//         comm = new AtomicBroadcastRing(node_id, nodes, conf, chance, delay);
//     } else if (broadcast_type == "BE") {
//         std::cout << "----- Executando Best Effort Broadcast -----\n";
//         comm = new ReliableComm(node_id, nodes, broadcast_type, conf, chance, delay);
//     } else if (broadcast_type == "UR") {
//         std::cout << "----- Executando Uniform Reliable Broadcast -----\n";
//         comm = new ReliableComm(node_id, nodes, broadcast_type, conf, chance, delay);
//     } else {
//         std::cerr << "Error: Unsupported broadcast type: " << broadcast_type << "\n";
//         return 1;
//     }

//     Application app(comm);
//     app.run(node_id);

//     return 0;
// }





// #include <iostream>
// #include "atomic_broadcast_ring.h"
// #include "reliable_comm.h"
// #include "keyValueStore.h"
// #include "loadGenerator.h"
// #include "performanceMonitor.h"
// #include "logger.h"
// int main(int argc, char* argv[]) {
//     // Initialize Atomic Broadcast Ring
//     //AtomicBroadcastRing atomicRing;

//     // Parse configuration file
//     std::map<int, std::pair<std::string, int>> nodes = {
//         {0, {"127.0.0.1", 3000}},
//         {1, {"127.0.0.1", 3001}},
//         {2, {"127.0.0.1", 3002}}//,
//         // {3, {"127.0.0.1", 3003}},
//         // {4, {"127.0.0.1", 3004}}
//     };

//     std::map<std::string, std::set<int>> groups = {
//         {"GroupA", {0, 1}},
//         {"GroupB", {1, 2}},
//         {"GroupC", {0}}
//     };

//     // Assume node ID is passed as an argument to the program
//     if (argc < 2) {
//         std::cerr << "Usage: " << argv[0] << " <node_id>" << std::endl;
//         return 1;
//     }

//     int process_id = std::stoi(argv[1]);

//     // Initialize AtomicBroadcastRing
//     AtomicBroadcastRing atomicRing(process_id, nodes, "config.txt", 0, 100, groups);


//     // Initialize KeyValueStore with AtomicBroadcastRing
//     KeyValueStore kvStore(atomicRing);

//     // Initialize Logger
//     Logger logger("performance.log");
//     logger.log("Application started");

//     // Initialize Performance Monitor
//     PerformanceMonitor monitor;

//     // Start monitoring performance
//     monitor.startMonitoring();

//     // Set up parameters for the load generator
//     int readWriteRatio = 70; // 70% writes, 30% reads
//     int totalOperations = 1000;
//     int numClients = 5;

//     // Initialize Load Generatorpwd
//     LoadGenerator loadGen(readWriteRatio, totalOperations, numClients, kvStore);

//     // Start generating load
//     std::cout << "Starting load generation..." << std::endl;
//     loadGen.generateLoad();

//     // Stop monitoring performance after load generation is complete
//     monitor.stopMonitoring();

//     // Log the completion of the process
//     logger.log("Application completed successfully");

//     // Print a final message
//     std::cout << "Application has completed. Check performance.log for details." << std::endl;

//     return 0;
// }




// #include "application.h"
// #include "reliable_comm.h"
// #include "atomic_broadcast_ring.h"
// #include "channels.h"
// #include <iostream>
// #include <fstream>
// #include <sstream>
// #include <map>
// #include <string>
// #include <algorithm>


// // Helper function to trim leading and trailing spaces
// std::string trim(const std::string& str) {
//     size_t first = str.find_first_not_of(" \t");
//     size_t last = str.find_last_not_of(" \t");
//     return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, (last - first + 1));
// }

// std::pair<std::map<int, std::pair<std::string, int>>, std::string> parseConfig(const std::string& config_path) {
//     std::ifstream config_file(config_path);
//     if (!config_file.is_open()) {
//         throw std::runtime_error("Failed to open configuration file: " + config_path);
//     }

//     std::map<int, std::pair<std::string, int>> nodes;
//     std::string broadcast_type;
//     std::string line;

//     while (std::getline(config_file, line)) {
//         std::istringstream iss(line);
//         std::string key, value;

//         if (line.find("nodes") != std::string::npos) {
//             // Extracting the part that contains nodes inside the braces
//             size_t start = line.find("{{") + 1;
//             size_t end = line.find("}}");

//             std::string nodes_data = line.substr(start, end - start);
//             nodes_data = trim(nodes_data);  // Trim spaces

//             // Parsing each node entry
//             std::istringstream nodes_stream(nodes_data);
//             std::string node_entry;
//             while (std::getline(nodes_stream, node_entry, '}')) {
//                 if (node_entry.find('{') != std::string::npos) {
//                     node_entry = node_entry.substr(node_entry.find('{') + 1); // Remove the '{' part
//                     node_entry = trim(node_entry);  // Trim spaces

//                     // Parse node details: ID, address, and port
//                     std::istringstream node_iss(node_entry);
//                     int node_id;
//                     char sep;
//                     std::string address;
//                     int port;

//                     node_iss >> node_id >> sep;  // Read the node ID and ignore the comma
//                     std::getline(node_iss, address, ':');  // Read address before ':'
//                     node_iss >> port;  // Read port

//                     // Add node to the map
//                     nodes[node_id] = {address, port};
//                 }
//             }
//         } else if (line.find("broadcast") != std::string::npos) {
//             size_t start = line.find('=') + 1;
//             size_t end = line.find(';');
//             broadcast_type = line.substr(start, end - start);
//             broadcast_type = trim(broadcast_type);  // Trim spaces around broadcast type
//         }
//     }

//     config_file.close();

//     // Debug prints to check parsed values
//     // std::cout << "Parsed broadcast type: " << broadcast_type << std::endl;
//     // std::cout << "Parsed nodes: \n";
//     // for (const auto& node : nodes) {
//     //     std::cout << "Node ID: " << node.first << ", Address: " << node.second.first << ", Port: " << node.second.second << std::endl;
//     // }

//     return {nodes, broadcast_type};
// }

// int main(int argc, char** argv) {
//     if (argc < 3) {
//         std::cerr << "Usage: " << argv[0] << " <node_id> <config_path>\n";
//         return 1;
//     }

//     int node_id = atoi(argv[1]);
//     std::string config_path = argv[2];

//     std::map<int, std::pair<std::string, int>> nodes;
//     std::string broadcast_type;

//     try {
//         auto config = parseConfig(config_path);
//         nodes = config.first;
//         broadcast_type = config.second;
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//         auto config = parseConfig(std::string("../config.txt"));
//         nodes = config.first;
//         broadcast_type = config.second;
//     }

//     // Debug prints to check what values are passed
//     std::cout << "Node ID: " << node_id << "\n";
//     std::cout << "Broadcast Type: " << broadcast_type << "\n\n";

//     std::string conf = "FULL";
//     int chance = 0;
//     int delay = 1;

//     ReliableComm* comm;

//     if (broadcast_type == "AB") {
//         std::cout << "----- Executando Atomic Broadcast -----\n";
//         comm = new AtomicBroadcastRing(node_id, nodes, conf, chance, delay);
//     } else if (broadcast_type == "BE") {
//         std::cout << "----- Executando Best Effort Broadcast -----\n";
//         comm = new ReliableComm(node_id, nodes, broadcast_type, conf, chance, delay);
//     } else if (broadcast_type == "UR") {
//         std::cout << "----- Executando Uniform Reliable Broadcast -----\n";
//         comm = new ReliableComm(node_id, nodes, broadcast_type, conf, chance, delay);
//     } else {
//         std::cerr << "Error: Unsupported broadcast type: " << broadcast_type << "\n";
//         return 1;
//     }

//     Application app(comm);
//     app.run(node_id);

//     return 0;
// }

