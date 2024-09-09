#include "subject.h"
#include <iostream>
#include <vector>


// Subject that will notify observers
class Subject {
public:
    void attach(Observer* observer) {
        observers.push_back(observer);
    }

    void detach(Observer* observer) {
        observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
    }

    void notify() {
        for (Observer* observer : observers) {
            observer->update();
        }
    }

private:
    std::vector<Observer*> observers;
};