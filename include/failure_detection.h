#ifndef FAILURE_DETECTION_H
#define FAILURE_DETECTION_H

#include<chrono>

class FailureDetection {
public:
    FailureDetection(double timeout);
    void starttime();
    bool timedout();
private:
    double max_time;
    double start;
};

#endif 
