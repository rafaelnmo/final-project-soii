#include "failure_detection.h"

FailureDetection::FailureDetection(double timeout) {
    max_time = timeout;
};

double getCurrentTimeInSeconds() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch(); // Duration since epoch in nanoseconds
    return std::chrono::duration<double>(duration).count(); // Convert to seconds as double
}

void FailureDetection::starttime() {
    start = getCurrentTimeInSeconds();
}

bool FailureDetection::timedout() {
    double now = getCurrentTimeInSeconds();
    double duration = (now - start);
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