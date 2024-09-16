#include<channel.cpp>
#include<failuredetection.cpp>
#include<utility>

using namespace std;

class ReliableComm{
    private:
        int process_id;
        map<int, pair<string, int>> nodes;
        set<int> delivered_messages;
        Channel channels;
        FailureDetection failure_detection;
        mutex mtx;
        condition_variable cv;
        queue<Message> message_queue;
    public:
        void send(int sender_id, vector<char> content){}
        void broadcast(vector<char> content){}
        Message receive(){}
        Message deliver(){}
        /*
        void listen(){}
        void send_message(int sender_id, vector<char> content){}
        bool is_delivered(int ???){}
        void mark_delivered(int ???){}
        */
        
};