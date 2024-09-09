#ifndef SUBJECT_H
#define SUBJECT_H

#include "observer.h"
#include <vector>

class Subject {
public:
    void attach(Observer* observer);
    void detach(Observer* observer);
    void notify();

private:
    std::vector<Observer*> observers; 
};

#endif 
