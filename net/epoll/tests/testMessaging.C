#include "epollservice/epollservice.H"

#include <iostream>
#include <unistd.h>

#include "MessageHandler.H"
MessageHandler listener;
DatagramHandler gramListener;

int main() {

    EpollActiveObject service("test");

    TcpConnectionServer* tcpServer = service.createTcpConnectionServer(4567, &listener);
    if (!tcpServer) exit(1);

    UdpUnicast* udpUnicast = service.createUdpUnicast("udp:/eth0/localhost/5678");
    if (!udpUnicast) exit(1);
    udpUnicast->addListener(&gramListener);

    McastSender* mcastSender = service.createMcastSender("udp:/eth0/239.1.1.1/6789");
    if (!mcastSender) exit(1);

    McastReceiver* mcastReceiver = service.createMcastReceiver("udp:/eth0/239.1.1.1/6789");
    if (!mcastReceiver) exit(1);
    mcastReceiver->addListener(&gramListener);

    service.start();

    //sleep(1);
    TcpConnection* tcpClient = service.createTcpConnection("tcp:/localhost/4567", &listener);
    //TcpConnection* tcpClient = service.createTcpConnection("tcp:/127.0.0.1/4567", &listener);
    if (!tcpClient) exit(1);

    testMsg1 tcpMsg(3, 5.1);
    testMsg1 udpMsg(6, 8.1);
    testMsg1 mcastMsg(9, 2.1);

    uint64_t counter = 0;
    while(service.isRunning()) {
        switch(counter % 3) {
        case 0: tcpClient->send((char*)&tcpMsg, sizeof(tcpMsg)); break;
        case 1: udpUnicast->send((char*)&udpMsg, sizeof(udpMsg)); break;
        case 2: mcastSender->send((char*)&mcastMsg, sizeof(mcastMsg)); break;
        }
        ++counter;
        if (counter > 60) service.stop();
        sleep(1);
    }
}
