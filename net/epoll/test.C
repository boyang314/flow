#include "epollservice.H"

#include <iostream>
#include <unistd.h>

StreamListener listener;

int main() {
    EpollActiveObject service("test");

    TcpConnectionServer* tcpServer = service.createTcpConnectionServer(4567, &listener);

    UdpUnicast* udpUnicast = service.createUdpUnicast("udp:/bond0/localhost/5678");

    McastSender* mcastSender = service.createMcastSender("mcast:/bond0/239.1.1.1/6789");
    McastReceiver* mcastReceiver = service.createMcastReceiver("mcast:/bond0/239.1.1.1/6789");

    service.start();

    TcpConnection* tcpClient = service.createTcpConnection("tcp:/localhost/4567", &listener);

    const char tcpMsg[] = "tcp::message";
    const char udpMsg[] = "udp::message";
    const char mcastMsg[] = "mcast::message";
    uint64_t counter = 0;
    while(service.isRunning()) {
        /*
        switch(counter % 3) {
        case 0: tcpClient->send(tcpMsg, sizeof(tcpMsg)); break;
        case 1: udpUnicast->send(udpMsg, sizeof(udpMsg)); break;
        case 2: mcastSender->send(mcastMsg, sizeof(mcastMsg)); break;
        }*/
        ++counter;
        if (counter > 12) service.stop();
        sleep(1);
    }
}
