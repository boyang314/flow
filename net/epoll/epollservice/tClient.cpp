#include "epollservice.H"

#include <iostream>
#include <unistd.h>
#include "MsgHeader.H"
#include <string>
#include <string.h>

#define MaxBufferLen 1024

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
    char buffer[MaxBufferLen];

    uint32_t counter = 0;
    while(service.isRunning()) {
        std::cout << "tcp client sendmsg " << counter << '\n';

        memset(buffer, 0, MaxBufferLen);
        std::string msg(counter, 'a');
        header.len_ = counter;
        memcpy(buffer, &header, sizeof(header));
        memcpy(buffer+sizeof(MsgHeader), msg.c_str(), header.len_);

        tcpClient->send((const char*)buffer, sizeof(header) + header.len_);

        sleep(1);
        ++counter; if (counter > 60) break;
    }
}
