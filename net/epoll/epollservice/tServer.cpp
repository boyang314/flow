#include "epollservice.H"

#include <iostream>
#include <unistd.h>

#include "MsgHeader.H"

struct HeaderHandler : public StreamListener {
    virtual void onPacket(const char* data, size_t len, const sockaddr_in& from, TcpConnection* conn) override {
        const MsgHeader* header = (const MsgHeader*)data;
        std::cout << "onPacket header.magic:" << header->magic_ << " header.len:" << header->len_ << " pktlen:" << len << std::endl;
    }
};

int main() {
    EpollActiveObject service("test");

    HeaderHandler handler;
    TcpConnectionServer* tcpServer = service.createTcpConnectionServer(4567, &handler);
    if (!tcpServer) exit(1);

    service.start();

    uint64_t counter = 0;
    while(service.isRunning()) {
        ++counter;
        sleep(1);
    }

}
