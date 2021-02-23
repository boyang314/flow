#pragma once

#include <string>
#include <thread>
#include <iostream>
#include <netinet/in.h> //sockaddr_in

class EpollService {
public:
    EpollService(int epoll) : epoll_(epoll) {} //error checking epoll_ <= 0?
    virtual bool initialize() = 0;
    virtual void onEpollOut() {}
    virtual void onEpollIn() = 0;
    virtual void onEpollError() = 0;
protected:
    int epoll_{0};
};

class TcpConnection;
struct StreamListener {
    virtual void onPacket(const char* data, size_t len, const sockaddr_in& from, TcpConnection* conn) {}
    virtual void onConnect(TcpConnection* conn) {}
    virtual void onDisconnect(TcpConnection* conn) {}
};

class TcpConnection : public EpollService {
public:
    TcpConnection(int epoll, const std::string& peerAddr, StreamListener* listener);
    bool initialize() override;
    void onEpollIn() override;
    void onEpollError() override;
    void send(const char* msg, size_t len);
};

class TcpConnectionServer : public EpollService {
public:
    TcpConnectionServer(int epoll, unsigned port, StreamListener* listener);
    bool initialize() override;
    void onEpollIn() override;
    void onEpollError() override;
};

class UdpUnicast : public EpollService {
public:
    bool initialize() override;
    void onEpollIn() override;
    void onEpollError() override;
    void send(const char* msg, size_t len);
};

class McastSender : public EpollService {
public:
    bool initialize() override;
    void onEpollIn() override;
    void onEpollError() override;
    void send(const char* msg, size_t len);
};

class McastReceiver : public EpollService {
public:
    bool initialize() override;
    void onEpollIn() override;
    void onEpollError() override;
};

class EpollActiveObject {
public:
    EpollActiveObject(const std::string& name);
    ~EpollActiveObject() { stop(); }

    TcpConnection* createTcpConnection(const std::string& peerAddr, StreamListener* listener);
    TcpConnectionServer* createTcpConnectionServer(unsigned port, StreamListener* handler);
    UdpUnicast* createUdpUnicast(const std::string& addr);
    McastSender* createMcastSender(const std::string& addr);
    McastReceiver* createMcastReceiver(const std::string& addr);

    void start();
    void stop();
    int isRunning() { return running_; }

    friend std::ostream& operator<<(std::ostream& os, const EpollActiveObject& obj);

private:
    void run();

    std::string name_;
    std::thread thread_;
    volatile int running_{0};
    int epoll_{0};
    const int max_epoll_events_{1024};
};

