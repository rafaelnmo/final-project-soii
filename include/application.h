#ifndef APPLICATION_H
#define APPLICATION_H

#include "reliable_comm.h"

class Application {
public:
    Application(ReliableComm* comm);
    void run(int process_id);

private:
    ReliableComm* comm;
};

#endif