#include <message.cpp>
#include <utility>
using namespace std;

class Channel{
    private:
        map<int, pair<string, int>> nodes;
        int sock;
        // TODO definir struct de my_addr
    public:
        void bind_socket(int sock) {}
        void send_message(int sender_id, vector<char> content) {}
        std::pair<Message, int> receive_message(){}
};