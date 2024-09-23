#include "failure_detection.h"
using namespace std::chrono;

FailureDetection::FailureDetection(double timeout) {
    max_time = timeout;
}

void FailureDetection::starttime() {
    start = high_resolution_clock::now();
}

bool FailureDetection::timedout() {
    double now = high_resolution_clock::now();
    double duration = duration_cast<milliseconds>(now - start);
    return (duration>max_time);
}

/*
    ==EXEMPLO DE USO==
    message = std::pair<Message, int> (Message(0, {}), 0) troca aqui pela mensagem de controle e o == por != para início/final da comunicação
    fdetec = new FailureDetection(10000);
    fdetec.starttime();
    while(!fdetec.timedout() && message==(Message(0, {}), 0){
        message = receive_message();
    }

    outra ideia seria iniciar start no construtor
    vantagem: não precisa rodar starttime(), código mais enxuto
    desvantagem: precisaria instanciar um novo FailureDetection antes de cada envio/recebimento
*/