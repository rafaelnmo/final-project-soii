#include "observer.h"
#include <iostream>
#include <vector>

class ConcreteObserver : public Observer {
public:
    void update() override {

        std::cout << "ConcreteObserver: Received update notification." << std::endl;
    }
};