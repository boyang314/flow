#include "epollservice.H"

#include <iostream>
#include <unistd.h>
#include "MsgHeader.H"

StreamListener listener;

int main() {
    EpollActiveObject service("test");
    service.start();

    //TcpConnection* tcpClient = service.createTcpConnection("tcp:/localhost/4567", &listener);
    TcpConnection* tcpClient = service.createTcpConnection("tcp:/127.0.0.1/4567", &listener);
    if (!tcpClient) {
        std::cerr << "failed to create tcp client\n";
        exit(1);
    }

    MsgHeader header;
    header.magic_ = 1234;

    uint32_t counter = 0;
    while(service.isRunning()) {
        std::cout << "tcp client sendmsg " << counter << '\n';
        header.len_ = counter;
        tcpClient->send((const char*)&header, sizeof(header));
        sleep(1);
        ++counter; if (counter > 60) break;
    }
}
