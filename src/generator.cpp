#include "generator.h"

generator::generator(int num_threads, int write_chance, int total_ops, ReliableComm* comm)
{
    std::srand(std::time(nullptr));
    bd_id = 0;
    comm = comm;
    write_chance = write_chance;
    total_ops = total_ops;

    // for(int i=0; i<num_threads; i++){
    //     std::thread t(&generator::generate_message, this);
    //     generator_threads.push_back(std::move(t));
    //     generator_threads[i].detach();
    // }
    std::thread t(&generator::generate_message, this);
    generator_threads.push_back(std::move(t));
    generator_threads[0].detach();
}

void generator::run() {
    send_message();
}

generator::~generator()
{

}

void generator::generate_message(){
    //for(int i=0; i<total_ops; i++){
    for(int i=0; i<total_ops; i++) {
        std::cout << "Generating message " << i << std::endl;
        // Generate a message
        int r = std::rand() % 100;
        Request req = Request(i, i, "SET"); // Default to write operation
        if (r>write_chance){
            // Read operation if chance happens
            req.type = "GET";
        }

        // Add the message to the queue
        queue_mtx.lock();
        message_queue.push(req);
        cv_send.notify_all();
        queue_mtx.unlock();

        int rand_sleep = rand()%(200-10 + 1) + 10;
        usleep(rand_sleep);
    }
}

void generator::send_message(){
    for(int i=0; i<(total_ops*int(generator_threads.size())); i++){
        std::cout << "Sending message " << i << std::endl;
        // fetch message
        std::unique_lock<std::mutex> lock(mtx);
        cv_send.wait(lock, [this] { return !message_queue.empty(); });

        queue_mtx.lock();
        Request req = message_queue.front();
        message_queue.pop();
        queue_mtx.unlock();

        // Send the message
        int result = comm->send(bd_id, req.serialize());

        // If successful, receive the response
        if (result) {
            Message response = comm->receive();
            if (response.msg_type=="ERR") {
                std::cout << "\n\n ERROR: Nothing to receive \n";
                continue;
            }
            std::cout << "\n\nReceived message from process " << response.sender_address << ": \n";
            for (auto byte : response.content) {
                std::cout << byte << std::endl;
            }
        }
    }
}