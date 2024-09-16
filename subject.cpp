#include <observer.cpp>
#include <utility>
using namespace std;

class Subject{
    private:
        vector<Observer> observers;
    public:
        void attach(Observer target){}
        void detach(Observer target){}
        void notify(){}
};